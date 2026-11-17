# Unchained: PC port features to do

What a good PC recomp release has, as the user sets the bar from the projects they contribute to and their own
SuperWonders: every item that applies to Whiplash, as such ports have it or adapted to an original Xbox, 4:3, D3D8
game. The numbers are the list's own. Status: `[ ]` to do, `[~]` started or partly there,
`[x]` done. "SDK" marks what the XboxXBERecomp runtime (`sdk/runtime/app/app.h` Settings) already gives.

Check every visual item against the reference (`C:\Unchained-private\reference\whiplash-reference-4k.mkv`, the PS2
version, so Xbox differences can be legitimate). Every performance item is measured with a per-present frame-time
suite (1% / 0.1% lows, hitches over 25/33/50/100 ms), never an fps average.

## Applies directly

### Core
- [x] **2. Game files setup** - first run asks for the disc image (.iso) or extracted folder, checks the XBE SHA-1,
  copies to the data folder, recompiles once with a progress screen. Done in the Configuration app's GAME FILES page
  (src/launcher/setup.cpp). Evidence: a first run beside no game files opens on it; a changed default.xbe is refused
  with both SHA-1s; the real folder (1.5 GB) is checked and copied ("FILES IN PLACE"). The code build runs in
  the same page (L5's evidence).
- [~] **4. Whole game finishable** - training level plays; the rest untested. Track a playthrough per level.
- [~] **5. Sound and voices correct** - sound bank plays; spoken dialogue (streamed `vo/VO_*`) never plays
  (docs/STATUS.md).
- [x] **8. One supported version** - refuse an XBE whose SHA-1 is not `3ea30323cec5734ddd4d535b5a269786b6900dc4`,
  with a clear message. SDK `GameInfo::one_release`. Evidence: a copy beside a one-byte-changed default.xbe shows
  "is a different release of the game ... Expected ... 3ea30323... Found 34a323e0..." and does not start (log:
  "another release ... refused").

### Platforms and input
- [~] **10. Any SDL controller** - SDL3 pad to XInput HLE is up; confirm by button position, every pad open, the last
  one used is read.
- [~] **11. Camera speed and vibration strength** - `camera_speed` (percent), `invert_camera_x`/`_y`, `vibration`
  (percent) in `src/common/controls.cpp`. Built, not yet felt in play (the user found the camera "weird and off").
- [ ] **14. Handheld preset** - detect PC handhelds; 30/40/45/60 fps and the screen's own size.
- [x] **15. Button prompts for the device in use** - Xbox, PlayStation or keyboard glyphs in the game's prompts,
  switching live. The prompts are characters in the game's font (texture `1a4d811ab994c436`); `src/common/prompts.cpp`
  draws its button row again, 4x, through the SDK's new `GameInfo::texture_image`: today's Xbox pad (coloured
  letters on dark discs, LB RB LT RT; scenario `prompts_xbox`; `button_prompts` 5 keeps the console's own glyphs),
  PlayStation symbols, Nintendo letters by position, or key caps with the keys bound in `[keys]`, made in code (no art). `button_prompts` 0 follows
  the device in use (the last controller touched, by SDL's gamepad type, or the keyboard and mouse; the keyboard at
  start with no controller). Scenarios `prompts_ps`, `prompts_nintendo`, `prompts_auto` (Space / Shift caps) in the
  "Play selected game?" dialog; `prompts_switch`: PlayStation then Nintendo in the same dialog of one run, the switch
  in a frame no longer than a capture's alone (91.9 vs 91.1 ms).
  The popup icons too (user, 2026-11-16: "some of the glyphs ... that pop up at the top of the screen" were missed):
  a scan of every texture through the menus, the lobby and the lab (`dump_lobby`, `dump_lab`, 311 textures) found the
  button glyphs only in the font and in the 32x32 popup icons, a green box round a button's disc (the lab's Y,
  `a2b608d7ea42f82e`; the "i" box beside it is no button). The icons are found by what is in them (the box's ring
  green, the disc's hue: yellow Y, green A, red B, blue X, white the White button), so the other buttons' icons are
  redrawn when the game shows them; the SDK's `texture_edits` now gets the texture's size to choose which to look
  at. Scenarios `icons_ps`, `icons_kb`, `icons_xbox`: the lab's popup a triangle, an R key cap, the Y.
- [x] **16. VRR and VSync modes** - `vsync_mode` = off, on, adaptive (on while frames keep up, a tear instead of a
  held frame), fast (no wait, no tear) or vrr (tearing allowed, capped 3 under the refresh). Evidence: scenarios
  `vsync_*` on this PC's 280 Hz monitor, uncapped: on 280 fps, adaptive 295, off 572, fast 559, vrr 277; no hitches.
- [x] **17. Window modes** - SDK: fullscreen, borderless, windowed, any resolution.

### Performance
- [~] **19. Steady frame rate** - measure with the frame-time suite at 60 and above. At 60 (lab, 2544x1401, render
  scale 3.97): 60.0 fps, 1% low 54-56, 0.1% low 47-55, no frame over 25 ms after the level loads; the recorded lobby
  run warm: hitches only at the level entry (53 and 77 ms). Above 60 in gameplay: not yet measured.
- [x] **20. Shader and pipeline cache** across runs - SDK: compiled shaders kept in `cache\shaders.bin`; the
  driver keeps its own pipeline cache. Evidence: recorded lobby run twice cold and twice warm
  (`turns\tests-20261116-055150`): worst frame 204-208 ms cold, 76-77 ms warm; 0.1% low 8 to 17-18 fps, 1% low 33 to
  41-43; hitches over 100 ms 2 to 0. The two left (53 and 77 ms) are the level entry (map open, shadow map made).
- [x] **21. Precompile every shader** before play (from a recorded list) - shaders compile once per PC into
  cache/shaders.bin; every pipeline is listed in cache/pipelines.bin and made ahead on worker threads at start (log
  "29 of 29 listed pipelines made ahead in 10 ms on 4 threads"). Recorded lobby run, warm, twice each (`lobby_warm`
  vs `lobby_warm_noahead`): 1% low 42.9/42.3 vs 42.7/42.3, 0.1% 18.2/18.3 vs 18.0/17.8, worst 77/78 vs 79/81 ms, 0
  frames over 100 ms. The two warm hitches are the game's level load at loading-screen cuts (`lobby_loads`: 20.1 s,
  "Play selected game?" to the falling animation, `map	raining.mxb` opened, 55 ms; 23.1 s, the animation to the lab,
  the level's textures arriving, 78 ms), not shaders or pipelines (`lobby_perf`: 0 shaders, 0 pipelines those
  seconds). A first run on a PC (`lobby_cold`) still compiles what it first sees, at those same cuts (214 and 124 ms):
  the shaders come from the game as it draws, so none ship.
- [~] **22. 30 to 360 fps at the right game speed** - SDK has an fps cap (60 by default); Whiplash's game logic speed
  above its own rate: the intro and the attract timer keep real time uncapped (scenarios `speed60` and `speed_uncapped`:
  the same frames at 8, 14, 20 s at 60 and ~560 fps; attract movie at 56 s and 53 s). Gameplay physics and cutscenes
  at high rates still to check before the default cap goes above 60.
- [x] **23. Graphics presets** - `graphics_preset` = custom, original, low, high, max (src/common/game_info.cpp).
  Evidence: `turns\tests-20261116-070924`: max renders at 7.95x with 4096 shadows, 60 fps, 1% low 52.8, 0.1% low
  48.4, no hitches; original gives the 4:3 console picture. The advanced settings are the Configuration app's GRAPHICS page.
- [ ] **25. Faster release build** - profile-guided optimisation and ThinLTO for the runtime.
- [x] **26. Performance overlay** - fps, frame time graph, GPU use. F10 or `perf_overlay`: fps, frame time, a graph
  of 240 frames, 1% and 0.1% lows and hitches over the last 1000 frames, the render size (src/common/overlay.cpp).
  Evidence: scenario `overlay` (turns/tests-20261116-075548). GPU use is not shown yet.
- [x] **27. Benchmark routes** - `tools/tests/run_tests.py`: scenarios with scripted or recorded input
  (`record.ps1`, `UNCHAINED_INPUT_RECORD`), captures, reference comparison and per-present frame times (1%/0.1% lows,
  hitches, stutter). Evidence: `turns\tests-20261116-045759
eport.md`.

### Graphics
- [x] **28. Resolution and render scale** - SDK: `render_percent`. Fixed 2026-11-16: the scale followed the
  `width`/`height` setting, so a 640x480 setting in a 2544x1401 window rendered at 1.0 and was upscaled; it now
  follows the window (log: `render scale 3.975`), never under the console's, 200+ supersamples.
- [x] **29. FSR 1** upscaling - `upscaler = 1` (default): AMD FSR 1 EASU when the view renders smaller than it
  shows (SDK `post/present.hlsl`, AMD MIT headers). Evidence: `tools/tests` scenario `fsr` (render_percent 50).
- [x] **37. Anisotropic filtering** - SDK: `aniso`; Unchained defaults to 16x (log: `anisotropic_filtering = 16`).
- [x] **38. Sharpening** - FSR 1 RCAS, `sharpen` 0-100 (0 off). Evidence: scenario `fsr` (sharpen 50).
- [x] **39. Higher-resolution shadows** - `shadow_resolution` (default 2048, 0 follows the render scale) through
  `GameInfo::target_scale`. Evidence: log `unchained: shadow map at 2048x2048`.
- [~] **40. Field of view** - `fov` (vertical degrees, 60 the game's) replaces only the game's 60 degree projection, so
  cameras with their own angle keep it. Not yet checked in play or in cutscenes.

### Player features
- [x] **44. Direct start plus a Configuration app** - Unchained.exe starts the game when its files are there;
  `Unchained.exe --configure` (or a first run) opens the Configuration app (src/launcher, `GameInfo::launcher`).
- [ ] **45. One-click updates** from the Configuration app.
- [x] **46. Save profiles** - SDK `save_profile`: a named profile keeps its own disk (`profiles/<name>`, saves and
  their backups). Evidence: scenario `profile` - log "hard disk under ...profiles/test profile", partitions made.
- [x] **47. Save backups** at each start, the last 5 kept - SDK `save_backups` (5): E:\UDATA to
  `save backups\<date time>` before boot. Evidence: log `saves: backed up to ...\save backups\2026-11-16 051540`;
  three runs, three folders.
- [x] **48. Custom save folder** - SDK `save_folder` (the console's hard disk, saves included). Evidence: scenario
  `savefolder` - log "hard disk under C:/Unchained-private/turns/savefolder-test", and the game made its player data
  there (E/UDATA/45530011/.../game00.dat to game07.dat, SaveMeta.xbx).
- [ ] **50. Cheats overlay** - money, invincibility, unlocks. Not through the disc's Game.ini: its debug switches
  ("Player invincible", "Enable debug menus", "Disable Creature Shadows", "Hide All") are not in the retail xbe and
  are ignored (served rewritten, 2026-11-16: no change). Needs the game's own player and money fields.
- [x] **M. In-game menu** (user, 2026-11-16, through a side session: the settings that apply live, in game) - F1,
  or Back and Start together, opens `src/common/menu.cpp` over the game in the Configuration app's look (teal, the
  green frame and tab, the slanted words, Windows' Bahnschrift, Consolas and Impact): display (window mode,
  widescreen, render resolution, field of view, VSync, frame cap), graphics (upscaler, sharpening, filtering, clean
  edges, character shadows, draw distance, colour grading, colour vision), HUD and prompts, controls (camera speed,
  inversion, vibration). All apply at once; closing it (F1, Escape, B, or Back and Start) saves them. While it is
  open the game gets no controller or keyboard input, nor the presses that closed it; the controller drives the menu
  (d-pad or stick, A, B, LB / RB for pages). Scenario `menu`: F1 in the lab, the arrows and Enter to HUD &
  PROMPTS, F1 closes, the game goes on; frames 52-70 s worst 18.8 ms, none over 25 ms. Not yet: a real controller
  in hand, audio (its setting needs a restart).
- [x] **U. Clean menu and HUD edges** (user, 2026-11-16: "re render all the other letters and UI boxes ... to match
  how clean the glyphs are", then "apply it to letters and any other simple things ... including all the green
  old terminal like things") - SDK `ui_upscale`. A 2D draw is one into the frame writing no depth whose vertices are
  on the screen already (0 to 1 across, as Whiplash's 2D programs give them, or pretransformed), or that tests no
  depth: Whiplash draws its menus and HUD with depth tested, so the first version, which looked for depth off, caught
  nothing. Its textures, when they are flat art, are drawn 4x on the texture worker: up to 8 flat colours
  (premultiplied, see-through fills counted) cover 85% of the texels, colours between two flat ones are their
  anti-aliased blend and go to the nearer end, few neighbours a gentle step apart (gradients and shading: clouds 0.26,
  a shaded coin 0.08, the font 0.01, cut at 0.06) and no noise; the labels are scaled by Scale2x three times and
  averaged 2x2, each flat colour stays flat. `ui_survey` logs each 2D texture's decision: drawn 4x the font, the menu
  and dialog frames and tabs, the box pieces with their fills, the HUD's skull and sword boxes, the corner company name,
  the clock; left the TV static, clouds, the shaded coin, the save portrait, the scanlines. Scenarios `ui_off` /
  `ui_on` (dialog, lab HUD) and `prestart_ui_off` / `prestart_ui_on` (main menu): text and frames crisp, the HUD's
  green icon as solid shapes (its faintest specks under half brightness drop out). Frame times twice each: 1% low
  42.9/42.9 off vs 42.0/43.0 on, worst 77/77 vs 78/77 ms, 0 over 100 ms.
- [x] **H. HUD at the screen edges** (user, 2026-11-16: "align to edge of screen like modern HUDs") - each HUD piece
  keeps its 4:3 shape and goes to its own edges, past the console's TV safe margin (`hud_edge`, 0 to 100, default
  100): left and right pieces to the sides, top and bottom pieces up and down, menus and centred text centred, full
  width overlays and bars as drawn; text and bars inside a box (this frame's or last frame's) follow it. Works at 4:3
  too. SDK: per draw, from its vertices' range on the CPU (d3d12.cpp ChooseDrawSide). Evidence (lobby every 2 s,
  `turns\tests-20261116-065649`): health boxes in the top corners with their numbers, inventory at the left edge,
  chain counter in the bottom right corner, title menu whole. The money box is not yet seen in a capture.
- [ ] **54. Translated menus** for Unchained's own screens.
- [ ] **55. Discord Rich Presence**, off by default.
- [ ] **56. Steam Deck and Add to Steam.**

## Adapted to Whiplash

- [ ] **1. The whole game from your own disc** - one game, no DLC; ISO or extracted folder.
- [x] **6. Languages** - whatever text and voice languages the disc has, chosen in settings. This release has
  English only: no French, German, Spanish or Italian files in Xbox.rkv or .r01-.r09 (rkv-scan, 2026-11-16), and the
  kernel answers XC_LANGUAGE English. Nothing to choose.
- [~] **9. Platforms** - Windows first. Linux natively, in four stages (branch `linux`, SDK branch `linux`; Ubuntu
  26.04, clang 21, Ryzen 7 7800X3D, RX 9070 XT on RADV, Wayland):
  Five stages; the renderer stays portable from the start (no Linux-only assumptions in it, the window's Vulkan surface
  from SDL3 or a thin platform layer), since the last stage puts Windows on Vulkan too (user, 2026-11-16).
  1. [x] **Builds, links and boots** (2026-11-16). `cmake --preset linux-relwithdebinfo`, then `Unchained` and
     `libUnchained.so` in `out/build/linux-relwithdebinfo`. The recompiler's output is byte-identical to Windows'.
     The game code is a shared library: its function names are weak aliases of the bodies, so an override the
     executable exports (`-rdynamic`) comes first in the lookup and wins; the executable's own calls to bodies are
     weak references bound at their first call (`-z lazy`, the library loaded `RTLD_GLOBAL`), which is what the
     delay loaded DLL does on Windows. The first-start build runs as on Windows (xbe_recomp, then clang and
     ld.lld, the system's for now): 65 files in 16 s on 16 threads, stamped, loaded. Host: SDL3 window and events
     (`app/window_sdl.cpp`); the game's Win32 window code gets the same messages, virtual keys and key state from
     SDL (`app/win32_compat.h`); SDL3 audio (5.1, or the console's stereo fold); guest RAM a memfd mapped at both
     physical windows; POSIX sockets for system link (the guest still sees Winsock's numbers); a pipe for the
     relaunch handover; PNG through SDL and BC1-3 unpacked in software for texture packs and ui_upscale. Evidence:
     the first run built the game code, booted (guest threads, kernel, audio opened stereo, the DirectSound effects
     image), closed with Exit (0), no crash report. Nothing is drawn yet (`gpu/null_backend.cpp`).
  2. [ ] **Drawn, to the title.** The D3D12 renderer unchanged, on vkd3d (Direct3D 12 on Vulkan, LGPL-2.1, with
     its own HLSL compiler for the translated shaders): `D3D12CreateDevice` and `D3DCompile2` from vkd3d-utils, a
     Vulkan swap chain on the SDL window whose images `vkd3d_create_image_resource` wraps as the back buffers,
     presents through `vkd3d_acquire_vk_queue`; ComPtr and the few Win32 types from a small header; ImGui's DX12
     backend with its SDL3 one. Ubuntu packages vkd3d 1.2 (2020, no HLSL compiler), so 1.19 is built from its
     release (`-DXBE_VKD3D_DIR=`); running that build on the Ubuntu PC waits for the user's OK. Check: every
     generated shader compiles with vkd3d's compiler (the log names any that fail), F3 captures against Windows'.
  3. [ ] **Parity.** The present pass (FSR 1, gamma, colour grading), HUD placement, widescreen and ui_upscale come
     with the renderer. Still to port: the prompts' key and plate text (FreeType and a bundled font instead of
     GDI), the Configuration app and the first-start progress window (ImGui on SDL3), the boot splash, the menu's
     fonts (no Windows fonts folder), HDR (the swap chain's HDR10 colour space), exclusive fullscreen modes. Frame-
     time suite (1% / 0.1% lows, hitches, pacing) on both platforms, same scenes, Linux no worse.
  4. [ ] **Release: one AppImage**, as the Windows release is one installer exe: SDL3, vkd3d, and clang with
     ld.lld for the first-start build inside it; the player's data under `XDG_DATA_HOME` (an AppImage's own folder
     is read-only).
  5. [ ] **Windows on Vulkan** (user, 2026-11-16: "after the port to linux is done, backport vulkan to the windows
     version"; a later session of its own, after the Linux port, starting from the Linux report's hand-over: the
     branches, the renderer's state and its Windows gaps). The Vulkan renderer built and run on Windows too, D3D12 kept as a setting until Vulkan reaches parity,
     then the default. Check: the frame-time suite on Windows, Vulkan against the D3D12 build, same scenes, Vulkan's
     lows and hitches no worse; F3 captures match. This weighs on stage 2's choice: vkd3d keeps the D3D12 renderer
     and reaches Vulkan through a translation layer (on Windows too, built with MinGW or clang), while a native
     Vulkan backend behind `gpu::Backend` is more work up front but is the one renderer both platforms would share
     without a layer in between.
- [~] **18. Widescreen and ultrawide** - Whiplash ignores the widescreen TV flag (the SDK's old 16:9 just stretched
  it). Its projection (`0x1390C0`) has a vertical fov, so `src/game/widescreen.cpp` widens the game's 4:3 aspect by
  the output's: `widescreen = "expand"` (default) fills 16:9 up to 32:9, 2D stays 4:3 in the middle, full screen
  passes stay full width; "off" is 4:3, "console" the old flag. The 448-line crop is `GameInfo::view_h`. Evidence:
  2026-11-16 run at 2544x1401, title and training level (`turns\multi-20261116-040301`, `shown.bmp`): wide 3D,
  HUD in the 4:3 middle, round icons round.
  Pop-in at the widened edges (user, 2026-11-16): the game culled with its 4:3 view; the camera's aspect getter
  (0x17F380) now gives the widened aspect, so the view, its culling and the projection agree. Evidence: the lab's
  first frames draw 539 and 520 times instead of 483 and 454; the domed doorway at the right edge, missing before, is
  drawn (turns/tests-20261116-071923 against turns/tests-20261116-071335).
  Edge pop-in with a wider fov (user, 2026-11-16 evening, snips and the recording `runs/popin.txt`): wall lights,
  plants, the planter and palm, hanging platforms and the lab's crystal vanished and came back near the edges as the
  camera turned; wall light glows glitched and character shadows sometimes went. Cause: the `fov` setting (80 for
  the user). The projection hook draws at that fov, but the renderer builds the frustum it culls with (0x16F620:
  fov +0x518, aspect +0x51C, near +0x510, far +0x514; six planes that its sphere test 0x16D680 checks every room
  drawable against in 0x194200) from the camera's own 60 degrees. A hook on 0x16F620 now builds it with the
  projection's fov (and far, by the same rule), then puts the view's values back. At the default fov 60 the two
  already agreed, which is why runs at 60 looked clean. Evidence: the frustum's left and right planes are 113.4
  degrees apart at fov 80 and 16:9 (92.7 before, the 60 degree view); the user played it and everything popping is
  gone, glows and shadows included. Scenario `popin_fov80` replays the recording at the user's fov 80 and draw
  distance 291; `popin_fov80_old` is the same with `UNCHAINED_OLD_FRUSTUM=1` (the game's frustum) for A/B.
  Frame times, two runs each, fix against old (turns/tests-20261116-211408 to -211629): average 59.7/59.8 against
  59.8/59.8 fps, 1% low 29.9/31.0 against 30.2/31.6, p99 20.5/20.2 against 20.5/20.8 ms, hitches over 25 ms 13/12
  against 13/13. Worse: the one frame at the cut from the loading screen into the lab (present 1049, 17.7 s) takes
  132/127 ms against 90/93, so 0.1% low 10.0/10.4 against 11.3/12.6 and one frame over 100 ms. That is the level
  load of item 21 with 45% more draws (130680 against 86956 a second in the lab, the objects fov 80 shows that were
  culled before), their textures arriving in the first frame; play after it is the same. At fov 60 nothing changes.
  Still open: the light beams wall lamps throw straight up vanish at some distances, anywhere on screen; not this
  cull, not portals, not the frame rate. Findings and next steps: [LIGHT_BEAM_FLICKER.md](LIGHT_BEAM_FLICKER.md),
  probe `tools/probes/cull_probe.cpp`. (`popin_old` sets `UNCHAINED_NO_CAMERA_WIDE`, which no code reads any more,
  so it runs as `popin_new`.)
- [ ] **30-32. Modern upscalers and frame generation** - FSR 3/4, DLSS need motion vectors a D3D8 game does not make;
  later, from depth and camera reprojection.
- [~] **33. Draw distance** - `draw_distance` (percent, 100-1000) raises the cameras' far clip in the projection
  hook (src/game/widescreen.cpp). Indoors (lab, 400%) nothing changes and depth stays right; needs an outdoor level
  to show, and the game's own distance culling and level-of-detail distances are not found yet. Found 2026-11-16:
  the hook raises the far clip only for a projection given the game's 4:3 aspect, and with `widescreen = "expand"`
  the gameplay camera's arrives already widened (the aspect getter 0x17F380), so its far clip never changes there;
  only "off" and "console" get it. The cull frustum (0x16F620) follows the same rule. To make it apply, both have to
  change together, and depth precision checked.
- [~] **36. Effect toggles** - Whiplash's own: hyper-mode trail, fog, shadows, glow, screen effects. Done:
  `character_shadows` (the shadow map's floor draws left out through `GameInfo::skip_texture`; log confirms). To do:
  the rest, once each effect's draws are known.
- [ ] **41. Highest level of detail** at any distance.
- [~] **49. Accessibility** - done: colour vision filters (`colour_filter` 1-3, daltonization in the present pass,
  `colour_filter_strength`) and HUD off (`hud_visible`, edge pieces only, menus stay). Evidence: scenario `deutan`
  (turns/tests-20261116-072431): red sign shifted toward pink, HUD gone. To do: invincibility, damage scaling, text
  speed (game hooks).
- [ ] **52. Level and area select** in place of a teleport.
- [x] **53. Mods** - SDK: texture replacement (`textures\replacement\`) and dumping, documented in [MODS.md](MODS.md).
  A served-file hook for other mods is in the SDK (`GameInfo::disc_file`).

## Launcher, files and theme (as SuperWonders did)

- [~] **L1. Configuration app themed to Whiplash** (done: dark teal lab, green terminal frames with the game's tab
  and cut corner, slanted FREEDOM / CRAZY / MADNESS words, Windows' Bahnschrift, Consolas and Impact; seven pages:
  play, display, graphics, controls, sound and saves, game files, about; turns/config-pages.png. To do: the
  player's own disc art at runtime) - a lab terminal look: dark teal, the green terminal
  HUD frames, the "FREEDOM / CRAZY / MADNESS" word style for headings; no character or company names. No game art in
  the repo: our own drawing, colours and fonts; art from the player's own disc only at runtime after setup.
- [x] **L2. First-run flow** (pick, verify, copy, build the code with its progress in the app, then PLAY) - choose the disc image or folder, verify, extract to the data folder, recompile with a
  themed progress screen, then start straight into the game.
- [x] **L5. Installs as a good recomp release does** (user, 2026-11-16) - the release zip holds the program only (exe, a
  `Unchained Configuration.exe` beside it, the bundled compiler and recompiler); the setup checks the player's files
  and copies them into the data folder; the game code is built once on their PC; saves, settings, logs and caches
  live in the data folder, the exe's by default, `UNCHAINED_DATA_ROOT` moves it. Done:
  the Configuration exe (release.bat; PLAY starts Unchained.exe), the data root (log "Data: ..."), the code build
  inside the setup with its progress in the app (`app::StartGameCodeBuild`). Evidence: a release-like folder (exe
  and compiler only): setup copied the files, built the code in 35 s ("The game's code is built"), and the next start
  went straight into the game without a build.
- [x] **L4. Project conventions** - no game files in the repo or builds, a weighted status page with progress bars
  (`tools/todo_progress.py` into STATUS.md), a [FAQ](FAQ.md), a bug report template
  (`.github/ISSUE_TEMPLATE/bug_report.md`), [SETTINGS.md](SETTINGS.md).

## Later

Started, then set aside by the user (2026-11-16: "Color lut and HDR can wait for now too", "Photo mode can wait",
"lets leave mouse and keyboard out for now and pick it up later"): kept as they are, not counted in the progress.

- [~] **12. Keyboard and mouse** - every action rebindable in the settings file's `[keys]` table (keys and mouse
  buttons; `src/common/keyboard.cpp`), the mouse turns the camera (`mouse_camera`, `mouse_sensitivity`), left and
  right mouse are X and B. Built; scripted runs unaffected; the mouse camera not yet felt in play. The mouse camera still held the cursor in the middle of the window after the click-to-capture change (user, 2026-11-16), so `mouse_camera` is off by default until it is picked up again.
- [~] **34. Colour grading** - exposure, contrast, saturation, temperature, tint (`grade_*`) and LUT files
  (`colour_lut`: a .cube from luts/, picked in the Configuration app; scenario `lut`: a test split tone applied, log
  "colour LUT ... 17^3"). Missing: the HUD keeping its colours (it is graded with the picture).
- [~] **35. HDR10** - `hdr`: the finished SDR picture (frame and UI) to a 10 bit Rec. 2020 PQ swap chain, white at
  `hdr_paper_white`, highlights stretched toward `hdr_peak` by `hdr_highlights`; the swap chain switches each second
  as Windows' HDR or the setting changes; F3 and F12 save the SDR picture. Scenario `hdr` on an SDR display: falls back
  with the log line "HDR asked, but Windows has this display in SDR", frame times as `hdr-off` (1% low 31.6 vs 31.3,
  hitches 10/5/3/1 vs 10/4/3/1). Missing: a look on an HDR display.
- [~] **51. Free camera and photo mode** - HUD hidden, a picture key. Done: F12 saves the window's picture as a PNG
  in screenshots/ (scenario `screenshot`: a 2544x1401 PNG), `hud_visible` hides the HUD. To do: a free camera.

## Not applicable

43 (settings in the game's own Options) and L3 (settings split with the game's Options): the in-game menu (M) and
the Configuration app hold the settings (user, 2026-11-16: "We don't have to do the in game settings menu, overlay and
launcher options are fine"). 7 (voice-bank crash fix), 13 (gyro), 42 (ambient occlusion and volumetric fog as new effects, not planned),
57 (achievements: no save-progress format known yet), 58 (amiibo).
