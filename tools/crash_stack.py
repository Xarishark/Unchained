"""Map the guest stack a runtime crash log prints to recompiled functions and their config names.

usage: python tools/crash_stack.py <log> [config/game.toml] [generated/game]
"""
import bisect
import re
import sys
from pathlib import Path

log = Path(sys.argv[1]).read_text(errors='replace')
config = Path(sys.argv[2] if len(sys.argv) > 2 else 'config/game.toml')
gen = Path(sys.argv[3] if len(sys.argv) > 3 else 'generated/game')

starts = sorted({int(x, 16) for x in re.findall(r'0x([0-9A-Fa-f]{6,8})', (gen / 'x86_func_mapping.cpp').read_text())})
names = {}
for line in config.read_text().splitlines():
    m = re.match(r'(\w+)\s*=\s*0x([0-9A-Fa-f]+)', line.strip())
    if m:
        names[int(m.group(2), 16)] = m.group(1)

SECTIONS = [('.text', 0x11000, 0x2B4038), ('D3D', 0x2B4040, 0x2C61A0), ('DSOUND', 0x2C61A0, 0x2D0A54),
            ('XMV', 0x2D0A60, 0x2F8794), ('XPP', 0x2F87A0, 0x2FE858)]


def section(a):
    return next((n for n, lo, hi in SECTIONS if lo <= a < hi), '?')


for line in log.splitlines():
    if 'error: exception' in line or ' read ' in line or ' write ' in line or 'error: d3d' in line:
        print(line.split('] ', 1)[-1])
for a in [int(x, 16) for x in re.findall(r'\[esp\+[0-9A-F]+\] ([0-9A-F]{8})', log)]:
    if not any(lo <= a < hi for _, lo, hi in SECTIONS):
        continue
    f = starts[bisect.bisect_right(starts, a) - 1]
    print(f'  {a:08X}  {section(a):6} sub_{f:08X}  {names.get(f, "")}')
