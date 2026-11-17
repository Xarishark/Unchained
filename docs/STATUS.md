# Status

<!-- PROGRESS -->
PC port features from [TODO.md](TODO.md) (started items count half):

| Area | Progress | Done |
|---|---|---|
| Core | `███████████████░░░░░` | 75% (3 of 4) |
| Platforms and input | `█████████████░░░░░░░` | 67% (4 of 6) |
| Performance | `███████████████░░░░░` | 75% (6 of 8) |
| Graphics | `██████████████████░░` | 92% (5.5 of 6) |
| Player features | `███████████░░░░░░░░░` | 55% (6 of 11) |
| Adapted to Whiplash | `████████░░░░░░░░░░░░` | 41% (4.5 of 11) |
| Launcher, files and theme (as SuperWonders did) | `██████████████████░░` | 88% (3.5 of 4) |
| **All** | `█████████████░░░░░░░` | **65%** (32.5 of 50) |
<!-- /PROGRESS -->

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
- **SDK fix 3 (local commit `ace2fa0`, upstream PR candidate): the strobe.** The runtime presented on every `D3DDevice_Swap`. Whiplash calls `Swap(2)` (BYPASSCOPY) and then `Swap(4)` (FINISH) every frame, so every other picture was a buffer nothing was drawn into. Its output alternated pale blue and black: a visible strobe. Now `FINISH` alone doesn't flip, as in the library. `strobe-check.ps1` captures 40 frames in a row: 8 colour changes before, 0 after.
- **Now:** a steady screen in the game's own clear colour `EAF3F7` (pale blue). The one other draw per frame, `sub_1D0340`, is a GPU timing test (rdtsc around a pretransformed quad, the result stored in microseconds), not a fade.

### Why the front end draws nothing (2026-11-15)
- **The game isn't stuck.** It's single-threaded, the main thread sleeps about 91% of each frame (pacing), and it streams 32 KB archive chunks through `ArchiveRawFile` from the sound code (`0x1C4xxx`), most likely title music. After about 52 s idle it starts the attract movie.
- **Whiplash renders through raw push buffers.** Each frame `RunPushBuffer` runs a 644-byte buffer (`0x80850000`/`0x80859000`, made by `sub_2B7010` at render init). It **jumps** (`0x00862001`) into the game's 4 MB scene buffer at `0x80862000` (its `XMemAlloc(0x400000, 0xBC000000)`).
- **SDK change 4 (local `103e2db`):** the walker now follows NV2A jumps, calls and returns. The scene buffer then shows the front end:
  - quad lists (prim 8) with **inline vertex arrays** (4 quads of 96 bytes, one batch of 19,680 bytes; 24 bytes per vertex);
  - vertex formats set by **`SET_VERTEX_DATA_ARRAY_FORMAT` methods `0x1760`–`0x179C`** inside the buffer.

  The runtime runs neither, so it guesses the stride from a stale format and drops every draw ("EndPush primitive 8 dropped").
- **The fix is a real NV2A push-buffer interpreter in the SDK:** vertex array formats and offsets, inline arrays, index and draw-array methods, probably vertex program upload/selection (`0x0B00`, `0x1E94`, …), and texture and state methods. No upstream code or fork has one:
  - hk21x's fork replays draws a game *recorded through the D3D API* (`BeginPushBuffer`), which doesn't fit Whiplash's hand-written commands.
  - sp00nznet/xboxrecomp has a push-buffer executor, which is a design reference (MIT/LGPL parts; read, don't copy).
- Test scripts now close the game with `WM_CLOSE` (`Stop-Unchained`), so the buffered Info log lines are flushed. Killing outright lost the tail of every log.

### The intro renders (2026-11-15, SDK local `d95fdf2`)
A push-buffer interpreter in `RunCommands` runs the raw NV2A commands Whiplash writes:
- **Vertex programs:** program load/data/start (`0x1E9C`/`0x0B00`/`0x1EA0`) and execution mode (`0x1E94`).
- **Vertex arrays:** array formats (`0x1760`) and offsets (`0x1720`).
- **Textures:** stage offset/format/enable (`0x1B00`/`0x1B04`/`0x1B0C`), with a guest texture header per pushed texture.
- **Pixel combiners:** packed back into a `D3DPIXELSHADERDEF`.
- **Draws:** inline arrays (`0x1818`), array elements (`0x1800`/`0x1808`) and draw arrays (`0x1810`). Indexed vertices are gathered on the CPU into a packed layout.

**Result (measured off-screen):**
- About 2 s: a fade from black into the pale screen.
- About 5 s on: a 3D snowy scene with the studio logo, then the **Whiplash title logo**, at about 60–70 draws a frame with 0 dropped.
- `strobe-check.ps1` (whole-frame brightness over 40 captures): the largest frame-to-frame change is 5/255, with 0 strobe flips.

**Known gaps:**
- The text batch in the first seconds (820 quads, DXT font texture) doesn't show yet.
- Linear textures (pushed size/pitch) aren't handled.
- Texture stage states (address/filter, `0x1B08`/`0x1B14`) aren't applied.
- Indexed draws are de-indexed on the CPU; measure the frame times before moving this to the GPU.

Tooling:
- `C:\Unchained-private\strobe-check.ps1`: brightness-based strobe detection; run it before showing the user anything.
- `C:\Unchained-private\turn-build.ps1` rebuilds and boots off-screen.
- `sample-threads.ps1` samples a running process's busy threads (dbghelp + DLL exports).
- `tools/crash_stack.py` maps crash stacks to functions.
- The exe rebuilds `Unchained.dll` on first start after some builds ("Game code: building", about 20–40 s) and relaunches itself; the guard follows.

### Gameplay (2026-11-16)

Input (keyboard on port 0, `UNCHAINED_INPUT` scripts), saves (XAPI disc check passes, `sub_001336E9`), creatures
(dword-aligned pushed attributes), shadows (pushed render targets and clears), the cutscene letterbox (surface clip
scissor) and the strobe are fixed. Pushed draws take the pushed shader stage program (`0x1E70`); a pushed texture on a
frame buffer reads the one being drawn; a linear texture on a target uses the target's size.

Open:
- **Hyper mode shows no effect.** Reference (video 12:48 "MADNESS", 16:42 "CRAZY"): the scene stays normal, the
  characters get bluish ghost copies trailing them, the HUD does not smear. Per frame the game clears the back buffer's
  alpha (`0x80`), draws the characters into alpha only (colour write `8`, texture `817EE400`), blends a 320x224 copy of
  past frames over the screen (`lerp(screen, trail * v0 * 2, trail.a * v0.a * 2)`), then copies the screen into the
  320x224 target (`0xD7EA00`). Before the frame-buffer read fix it read the other (shown) buffer and looked like a
  smeared frame blend; now it reads the drawn one and nothing shows, so the trail's alpha is likely 0: check the
  alpha-only character pass's output alpha and that the 320x224 copy keeps it. User's call: fine for now, noted.
- **Redmond's voice lines** (most missing): `CDirectSoundStream_Pause` is now mapped (`0x2C6E96`); the game drops a
  line when `Pause(stream, 2)` fails. Not tested yet.
- **Camera "felt weird and off"** (user): not described yet.

## Next

1. **Hyper mode trail** (above), then the frame-time suite (per-present CSV, 1%/0.1% lows, hitches).
2. **Input to the title menu** (done: keyboard), then the first level load (done).
3. Frame-time suite (per-present CSV, 1%/0.1% lows, hitches) once it reaches gameplay, per CLAUDE.md.
4. **XMV attract movie:** the DirectSound stream it makes is null in `CDirectSoundStream_Process`. HLE XMV (the SDK has none; sp00nznet's toolkit or FFmpeg are references) or skip the attract mode.
5. Try the union of PR #27's and hk21x's 5659 signatures for the other D3D/DSound functions.
6. Name the rest in Ghidra (`analysis/whiplash.exe`), then run `xbe_symbols --sign` to make Whiplash-variant signatures for upstream.
7. Add `data_ranges` for the XMV decoder tables once the attract movie path is HLE'd.

## Systematic scan (2026-11-16)

A pass over the same bug classes as the fixes above. Tools are outside the repo in `C:\Unchained-private\audit\`:
`xbe.py` (image, sections, `.XTLID`, call sites), `dis.txt` (every instruction, taken from the recompiler's generated
comments), `xtlid.txt` (the 141 `.XTLID` exports, with names from every signature file's ids).

### 1. Library functions the matcher missed or misnamed
The game's own code calls 25 library addresses that `config/game.toml` doesn't name. Each one was checked:
- **Misnamed:** `0x2C7486` was matched as `IDirectSoundBuffer_SetFilter`. `.XTLID` says `0x0B0067`, which is
  `SetEG` (ids are alphabetical: SetCurrentPosition 0x64, SetEG 0x67, SetFilter 0x68, SetFrequency 0x6A). The game
  passes an envelope (amplitude EG, release 1, sustain 255) at `0x1C2D91`. **Renamed.** Both runtime functions are
  no-ops, so this doesn't change the sound.
- **Fixed:** `GetBackBuffer2` (`0x2B5DA0`) and `GetDepthStencilSurface2` (`0x2B5DF0`) ran the library's code, which reads
  CDevice fields. The runtime set the front-buffer field to the back buffer, so the game got the same address for
  both (`0x3AC154`/`0x3AC158`). Both are now mapped, and the field holds the front buffer (see class 3).
- **Harmless:**
  - `BlockUntilIdle` (`0x2B58F0`) jumps to the mapped `KickOffAndWaitForIdle`.
  - Resource AddRef/Release (`0x2B73C0`/`0x2B7400`) and `CreatePushBuffer` (`0x2B7010`) only touch guest headers.
  - `BeginPushBuffer`/`EndPushBuffer` (`0x2B70A0`/`0x2B7110`): the game records each whole frame, then runs it with
    `RunPushBuffer`.
  - `InsertCallback` (`0x2B9110`) adds a GPU timing callback (`0x1D06E0`). It uses `0x1D8C`/`0x1D90` as scratch
    space, and those are also the pushed clear-value registers. No pushed clear follows in a play frame.
  - The DSound stream thunks at `0x2C75B2`/`B7`/`0x2C7B10` jump to mapped functions.
  - The XMV calls are the attract movie.
- **Boot warning `dsound: exports running their original code`:**
  - `B00A3` (`0x2C648D`) is pure pitch math (it compares the rate against 48000). Only DSOUND's own code calls it.
  - `B0080` (`0x2C75C1`) is the `IDirectSoundStream_FlushEx` thunk, and only XMV calls it.
  - `B002D` and `B000C` point into data tables.

  None of these touch a host object.

### 2. Pushed state the renderer took from stale API state
A play-frame dump (`UNCHAINED_PB_DUMP_AT=55`, new) lists what Whiplash pushes per frame. The renderer already reads
blend, colour mask, alpha test, depth func/write and stencil func from the pushed registers. From the API state it
still reads cull, fill, stencil enable/fail, fog range, shadow func and texture factor:
- **Cull (fixed, SDK `a7202e9`):**
  - The game turns culling on and off 18 times a frame (`0x308`). It never pushes the culled face.
  - Its last API call, the startup GPU timing test `0x1D0340`, leaves the D3D cull mode at NONE. So no pushed draw
    was ever culled.
  - Pushed draws now take culling from the registers. The API cull and front-face states set those registers as
    the library's own `CullMode` does: `0x39C = FRONT` when the mode equals the front face.
  - Check: culling the other winding (`UNCHAINED_PB_CULL=ccw`) wipes out the title road and the lab. The derived
    clockwise culling matches the unculled picture with nothing missing.
  - The game pushes front face `0` once a frame. That isn't a valid value, so the API's front face is kept.
- **Not pushed in play:** stencil enable stays 0; fog mode and the shadow compare func aren't pushed; fill mode is
  set once through the API. No change was needed.

### 3. Frame-buffer addresses
The full-screen effects pass (`XBoxFullScreenFX::Apply`, `0x1D2A70`) samples the front buffer when its mode is 0
(`0x1D2CDC`, pitch `0xA00`). The game took that address from `GetBackBuffer2(-1)`, which gave the back buffer
(class 1). Now the front buffer's first address reads the frame shown last, and the back buffer's first address still
reads the one being drawn (SDK `a7202e9`). Whatever uses mode 0 (fades or the hyper trail) needs a play check.
`CopyRects` and `PersistDisplay` are never called.

### 4. Open problems
- **(a) Dark picture:** the game's own ramp (`0x16C400`, called at boot with brightness 5.0) is
  `255 * (min(255, i*(b+7)/12)/255)^1.4`, which darkens the midtones (128 goes to 97). Mapping `SetGammaRamp`
  (`8ad0b8b`) applies that, so the picture gets **darker**, not brighter. The reference's lab scenes are dark teal
  too. Compare a play frame side by side with the reference before going further, and if the console is brighter,
  look at the TV output stage, not the ramp.
- **(d) Voice lines:**
  - `0x13D110` is the engine's second debug print, stripped to a bare `ret` in retail, with 358 callers. It now
    goes to the log as `engine:` lines; only direct calls are formatted, because the folded empty function is
    also a vtable slot.
  - Traces of the stream path show that music (`StrStream`, stereo 0x24000 rings, called from `0x2227EC`) and mono
    creature speech (called from `0x257A4F`) start and play at a peak of about 0.17.
  - Redmond's lines are `goe::LipStream` (vtables `LipStream` `0x305658`, `LipStreamInst` `0x304FF4`, `CreateInst`
    `0x1EE920`). At the lab's load, 9 LipStreams are created and their lip sync loads (`0x1C5BB0`, `0x1C6680`),
    but `CreateInst` was never called in about 3 minutes of scripted play. So nothing asked to play them.
  - Next: trace the cutscene lip-sync command (`CinLipSync` vtable `0x3094A4`, execute `0x2709B0`) and the
    `VoiceOverBeginsMsg` senders (`0x2072E3`, `0x242ABE`, `0x26205A`) at a scene known to speak, such as the fire
    machine at 13:03.
- **(f) XMV attract movie:** opened at 47 s and 79 s of an idle title screen with no crash. It looks fixed.
- (b) shadows, (c) hyper trail and (e) camera: not changed by this pass, apart from the front-buffer read in class 3.
