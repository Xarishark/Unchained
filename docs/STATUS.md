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

## Boots 4 to 17 (2026-11-15, as Unchained)

Each boot named the function or data the last crash pointed at. Names come from the 5659 layout's offsets, `.XTLID` export ids and call shapes; addresses and reasons are in `config/game.toml`:
- **D3D data:** `pDevice`, `DirtyFlags`, `RenderState`, `DeferredTextureState`, `Streams`, `FvfShader`.
- **D3D functions:** `SetTransform`, `BlockUntilVerticalBlank`, `SetScreenSpaceOffset`, `CDevice_KickOff`, `Swap`, `Clear`, `KickOffAndWaitForIdle`, `BlockOnTime`, `BlockOnResource`, `BeginPush`, `MakeSpace`, `MakeRequestedSpace`, `RunPushBuffer`.
- **DSound functions:** `SetPlayRegion`, `StopEx`, `SynchPlayback`.

**DSound `.XTLID` ids are alphabetical** (SetFrequency 0x6A, SetLoopRegion 0x6E, SetMixBins 0x71, SetMixBinVolumes 0x72, …). That allows filling gaps by counting.

**SDK fix (local commit `d592d5a` on the sdk branch, upstream PR candidate):** the runtime allocated its 4 MB DrawVerticesUP ring from the title's 64 MB. Whiplash sizes its heap to the free memory, so the ring was null on the first UP draw and the copy crashed. The ring now comes from system memory (`0xB0000000`).

**Now (boot 17):** the game runs about 54 s presenting frames: a solid light-blue clear, then black for a while. Then the **XMV player** (`0x2D1381`, XTLID 0x0E0008) crashes in `CDirectSoundStream_Process` reading guest 0x12. Nothing but the clear colour is visible yet.

## Boots 18+ (2026-11-15): off-screen tests, verbose log, the fade quad

- **Test runs never reach a monitor.** The bring-up build strobes between the light-blue clear and black, so:
  - `UNCHAINED_OFFSCREEN=1` (`src/common/game_info.cpp`) pins the game window far off-screen.
  - `offscreen-guard.ps1` kills the game within 100 ms if its window (class `Unchained`) is ever on a monitor.
  - `shot.ps1` captures the window with PrintWindow and can post F3 for a frame capture (`-F3At`, `-CaptureDir`).
- **`UNCHAINED_VERBOSE_LOG=ON`** (CMake option, set in the local build) compiles the SDK's `log.cpp` without NDEBUG. Release builds then log Info/Debug lines (and `[xbe] trace`) to `logs/unchained_NNN.log`.
- **What the verbose log shows:**
  - Startup (`sub_249E0`) runs to the end and opens `Xbox.rkv` + `r01`–`r09` at 1.14 s.
  - XAPI asks to relaunch at boot (the startup check the xiso tool patches), and the runtime refuses.
  - Five DSound exports still run their own code (XTLID `B00A3`, `B0082`, `B0080`, `B002D`, `B000C`).
  - The main loop (`sub_222D4` → `[vtable+0xC]`) presents frames. After about 52 s idle it opens `movies\attract.xmv` (the attract mode) and crashes in the XMV player.
- **SDK fix 2 (local commit `9e13a11`, upstream PR candidate):** immediate mode with a pretransformed FVF (XYZRHW) was dropped ("not supported"). The slots are now packed in the FVF layout and drawn as `DrawVerticesUP` draws.
- **Now:** each frame has one draw, the game's full-screen fade quad (FVF `0x144`), which now shows black over the light-blue clear. Nothing else draws, so the front end isn't putting anything on screen; the next step is the game's front-end state.

Tooling:
- `C:\Unchained-private\turn-build.ps1` rebuilds and boots off-screen.
- `sample-threads.ps1` samples a running process's busy threads (dbghelp + DLL exports).
- `tools/crash_stack.py` maps crash stacks to functions.
- The exe rebuilds `Unchained.dll` on first start after some builds ("Game code: building", about 20–40 s) and relaunches itself; the guard follows.

## Next

1. **Front end:** why only the fade quad draws. Trace the main loop's state (`sub_16C650`, which draws the fade) and what it waits on. Check the archive reads (`ArchiveRawFile`) and the `firstscreen` package load.
2. **XMV attract movie:** the DirectSound stream it makes is null in `CDirectSoundStream_Process`. HLE XMV (the SDK has none; sp00nznet's toolkit or FFmpeg are references) or skip the attract mode.
3. Try the union of PR #27's and hk21x's 5659 signatures for the other D3D/DSound functions.
4. Name the rest in Ghidra (`analysis/whiplash.exe`), then run `xbe_symbols --sign` to make Whiplash-variant signatures for upstream.
5. Add `data_ranges` for the XMV decoder tables once the attract movie path is HLE'd.
