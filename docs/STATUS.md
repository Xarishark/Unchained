# Status

The full plan is in `PLAN.md` in the analysis folder (outside this repo).

## Phase 0: setup (2026-11-15)

**Done:**
- Repo created from the XboxXBERecomp `templates/game`, renamed to Whiplash, with the SDK as `sdk/`.
- **SDK pin:** a *local* branch `whiplash-pr27` = upstream `main` `cc9edda` + [PR #27](https://github.com/TGP482/XboxXBERecomp/pull/27) (5659 signatures). The PR's `layout.toml` conflicts with main's, and **main's GTA III layout is kept, because Whiplash matches it** (see below). This commit is not on GitHub, so a fresh clone must recreate it or re-pin the SDK to upstream `main`.
- `build.bat` checks the XBE's MD5. It accepts retail, rejects the xiso-patched copy with the byte to fix, and rejects anything else.
- `tools/rtti_dump.py` dumps RTTI from the XBE to `analysis/` (ignored by git). It finds 1,041 type descriptors, 1,040 vtables and 2,932 distinct virtual functions.
- `analysis/whiplash.exe` is a flat PE for Ghidra (`sdk/tools/symbols/xbe2pe.py`).

**The tools build with llvm-mingw.** `xbe_symbols` and `xbe_recomp` both build and run with the llvm-mingw toolchain (`C:\SuperWonders-private\toolchain`). The game executable itself needs Visual Studio (MSVC linker flags, the Windows SDK and CRT), which this PC doesn't have yet.

## XDK symbols

`xbe_symbols` with PR #27's signatures finds **75 of 248 functions and 5 of 13 data**. 41 of the misses don't apply (Bink, XNet, sockets, XOnline: Whiplash has no networking or Bink). That leaves **140 relevant misses**: 66 `D3DDevice_*`, 12 `D3D_*`, 45 DirectSound, 7 CRT math, plus `XInitDevices`, `XAudioDownloadEffectsImage` and `XPP_MemoryUnitType`.

**The cause is that 5659 D3D libraries aren't all the same build.** The `CDevice` field offsets in main's 5659 layout (proven on GTA III) and in PR #27's (Crimson Skies) differ by 4 or 0x10 bytes. Whiplash's D3D section uses main's values:

| field | main / GTA III | hits in Whiplash D3D | PR #27 / Crimson Skies | hits |
|---|---|---|---|---|
| RenderTarget | 0x1A04 | 17 | 0x19F4 | 0 |
| SwapCount | 0x2478 | 9 | 0x2468 | 0 |
| Viewport | 0xEE0 | 9 | 0xED0 | 0 |
| Transforms | 0xC60 | 5 | 0xC50 | 0 |

The hk21x fork's 5659 signatures (from PGR2) do no better: 8 more functions, with the same D3D gap, and its layout also differs from Whiplash's. So the missing D3D/DSound functions have to be named by hand from Whiplash in Ghidra, then signed with `xbe_symbols --sign`. That would give upstream a second 5659 D3D variant, the GTA III/Whiplash one.

## First recompile (no link yet)

`xbe_recomp` runs cleanly (exit 0, 115 MB of C++ in 64 partitions):
- 25,805 functions, 862,790 instructions, 2.79 of 3.07 MB of code bytes covered.
- 155 jump tables and 112 C++ catch blocks.
- Discovery: 597 functions are found from direct calls, 10,088 pointer seeds are accepted (1,205 rejected), gap fill adds 5,066, and there are 4,767 overlapping decodes.
- **About 232 unimplemented legacy opcodes** (`les`, `aaa`, `bound`, `arpl`, `insb`, …) in 46 functions. That's data decoded as code:
  - 173 in the **XMV** section (WMV decoder tables). It only serves `attract.xmv` and will be replaced by an HLE player, so it can be marked as data.
  - 50 in `.text`, mostly around `0x1F5C70`–`0x1F8120`, plus 7 in DSOUND and 2 in D3D. These need checking in Ghidra and then `data_ranges`.

## First builds and boots (2026-11-15)

- The full build works with Visual Studio Community 2026 (v18) + Clang 23. The first build took about 17 minutes; a config-only rebuild takes about 10.
- Run `.\build.bat`: this PC doesn't search the current folder for commands, so plain `build.bat` isn't found. The template's `.bat` files also have to be CRLF, so re-checkout them if they arrive LF-only.
- **Boot 1:** an access violation reading `0xFED00000` (the OHCI USB controller) inside `XPP`, because `XInitDevices` was unmatched. **Fixed:** `XInitDevices = 0x2F95A4`, found from the call at `0x1A6E2A` with `XPP_GamepadType`.
- The vertex shader microcode in `.text` is now marked as `data_ranges` (28 shaders).
- **Boot 2:** gets past input init, then writes to `0xFD001804` (NV2A PCI command mirror) inside the game's own D3D: `Direct3D_CreateDevice` was unmatched. **Fix staged, not yet built:** `Direct3D_CreateDevice = 0x2B8180`. The game calls it from `sub_001D01A0` (render device init), and hk21x's signatures match the same address.
- Expect more D3D/DSound traps until the ~140 unmatched functions are named. Using the union of PR #27's and hk21x's signatures would add 8, so try that first.
- **Boot 3** (with `Direct3D_CreateDevice` named): the runtime's D3D now takes over. It logs that `D3D_pDevice`, `D3D_DirtyFlags`, `D3D_DeferredTextureState`, `D3D_RenderState` and `D3D_Streams` are unknown for this image, then exits with `C0000409`. Those five D3D data symbols are next.

## Next

1. Find the five D3D data symbols (the hk21x signatures read some of them from functions they match) and boot again.
2. Try the union of PR #27's and hk21x's 5659 signatures for the other D3D/DSound functions.
3. Name the rest in Ghidra (`analysis/whiplash.exe`), then run `xbe_symbols --sign` to make Whiplash-variant signatures for upstream.
4. Add `data_ranges` for the XMV decoder tables once the attract movie path is HLE'd.
