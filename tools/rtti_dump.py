"""Dump MSVC x86 RTTI from an original Xbox XBE: classes, base classes, vtables and their slots.

usage: python tools/rtti_dump.py assets/default.xbe analysis/rtti.json [analysis/classes.txt]

Reads only the user's own XBE. The output is analysis data (addresses and the type names the binary carries)
for Ghidra and for naming hooks; it stays out of the repository (analysis/ is ignored).
"""
import json
import struct
import sys

xbe = open(sys.argv[1], 'rb').read()
if xbe[:4] != b'XBEH':
    sys.exit('not an XBE')
base, = struct.unpack_from('<I', xbe, 0x104)
nsec, secaddr = struct.unpack_from('<II', xbe, 0x11C)

sections = []
for i in range(nsec):
    flags, va, vsize, raw, rsize, name_va = struct.unpack_from('<IIIIII', xbe, secaddr - base + i * 56)
    name = xbe[name_va - base:].split(b'\0', 1)[0].decode()
    sections.append((name, va, vsize, raw, rsize, flags))


def section_of(va):
    for s in sections:
        if s[1] <= va < s[1] + s[4]:
            return s
    return None


def u32(va):
    s = section_of(va)
    if not s or va + 4 > s[1] + s[4]:
        return None
    return struct.unpack_from('<I', xbe, s[3] + va - s[1])[0]


def cstr(va, limit=512):
    s = section_of(va)
    if not s:
        return None
    off = s[3] + va - s[1]
    return xbe[off:off + limit].split(b'\0', 1)[0].decode('latin1')


# XBE section flags mark .rdata and .data executable too, so code is told apart by section name
CODE_SECTIONS = {'.text', 'D3D', 'DSOUND', 'XMV', 'XPP'}


def executable(va):
    s = section_of(va)
    return s is not None and s[0] in CODE_SECTIONS


data_sections = [s for s in sections if s[0] in ('.rdata', '.data')]

# TypeDescriptor: pVFTable(type_info), spare, name ".?AV...@@" / ".?AU...@@"
type_descs = {}
for name, va, vsize, raw, rsize, flags in data_sections:
    blob = xbe[raw:raw + rsize]
    pos = 0
    while True:
        pos = blob.find(b'.?A', pos)
        if pos < 0:
            break
        if blob[pos + 3:pos + 4] in (b'V', b'U') and pos >= 8:
            td = va + pos - 8
            type_descs[td] = cstr(va + pos)
        pos += 3


def demangle(raw_name):
    # .?AVName@ns1@ns2@@ -> ns2::ns1::Name (templates kept mangled)
    body = raw_name[4:]
    if body.endswith('@@'):
        body = body[:-2]
    if body.startswith('?$'):
        return raw_name[4:]
    return '::'.join(reversed([p for p in body.split('@') if p]))


def class_hierarchy(chd):
    sig, attrs, count, arr = (u32(chd + o) for o in (0, 4, 8, 12))
    if sig != 0 or count is None or count > 64 or arr is None:
        return None
    bases = []
    for i in range(count):
        bcd = u32(arr + i * 4)
        if bcd is None:
            return None
        td = u32(bcd)
        if td not in type_descs:
            return None
        bases.append({'type': demangle(type_descs[td]), 'contained': u32(bcd + 4),
                      'mdisp': struct.unpack('<i', struct.pack('<I', u32(bcd + 8)))[0]})
    return {'attributes': attrs, 'bases': bases}


# CompleteObjectLocator: signature 0, offset, cdOffset, pTypeDescriptor, pClassHierarchyDescriptor
locators = {}
for name, va, vsize, raw, rsize, flags in data_sections:
    for off in range(0, rsize - 20, 4):
        sig, offset, cd, td, chd = struct.unpack_from('<IIIII', xbe, raw + off)
        if sig == 0 and td in type_descs and section_of(chd):
            h = class_hierarchy(chd)
            if h:
                locators[va + off] = {'type': demangle(type_descs[td]), 'offset': offset, 'hierarchy': h}

# vtable: the dword before slot 0 points at the locator
vtables = []
for name, va, vsize, raw, rsize, flags in data_sections:
    for off in range(0, rsize - 8, 4):
        col = struct.unpack_from('<I', xbe, raw + off)[0]
        if col in locators:
            vt = va + off + 4
            slots = []
            while True:
                f = u32(vt + len(slots) * 4)
                if f is None or not executable(f):
                    break
                # the next vtable's locator pointer ends this one
                if len(slots) and u32(vt + len(slots) * 4) in locators:
                    break
                slots.append(f)
            if slots:
                loc = locators[col]
                vtables.append({'type': loc['type'], 'this_offset': loc['offset'], 'vtable': vt,
                                'locator': col, 'slots': slots})

classes = {}
for loc in locators.values():
    c = classes.setdefault(loc['type'], {'bases': [b['type'] for b in loc['hierarchy']['bases'][1:]],
                                         'vtables': []})
for vt in vtables:
    classes.setdefault(vt['type'], {'bases': [], 'vtables': []})['vtables'].append(
        {'address': vt['vtable'], 'this_offset': vt['this_offset'], 'slots': vt['slots']})

virtuals = sorted({f for vt in vtables for f in vt['slots']})
out = {'xbe_base': base, 'type_descriptors': len(type_descs), 'classes': classes,
       'virtual_functions': virtuals}
json.dump(out, open(sys.argv[2], 'w'), indent=1, sort_keys=True)

if len(sys.argv) > 3:
    with open(sys.argv[3], 'w') as f:
        for name in sorted(classes):
            c = classes[name]
            vts = ', '.join(f"{v['address']:08X}+{v['this_offset']:X}[{len(v['slots'])}]" for v in c['vtables'])
            f.write(f"{name}" + (f" : {', '.join(c['bases'])}" if c['bases'] else '') + (f"   vt {vts}" if vts else '') + '\n')

print(f'{len(type_descs)} type descriptors, {len(locators)} object locators, {len(classes)} classes with '
      f'locators, {len(vtables)} vtables, {len(virtuals)} distinct virtual functions')
