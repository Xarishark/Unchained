# Unchained: PC port features to do

Modelled on Savage Wind's feature set (upstream `test`, Alpha 6, 2026-11-16) the way SuperWonders was: every item
that applies to Whiplash, either as Savage Wind has it or adapted to an original Xbox, 4:3, D3D8 game. Numbers are
Savage Wind's (its feature list as given to the user). Status: `[ ]` to do, `[~]` started or partly there,
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
- [~] **12. Keyboard and mouse** - every action rebindable in the settings file's `[keys]` table (keys and mouse
  buttons; `src/common/keyboard.cpp`), the mouse turns the camera (`mouse_camera`, `mouse_sensitivity`), left and
  right mouse are X and B. Built; scripted runs unaffected; the mouse camera not yet felt in play.
- [ ] **14. Handheld preset** - detect PC handhelds; 30/40/45/60 fps and the screen's own size.
- [ ] **15. Button prompts for the device in use** - Xbox, PlayStation or keyboard glyphs in the game's prompts,
  switching live.
- [x] **16. VRR and VSync modes** - `vsync_mode` = off, on, adaptive (on while frames keep up, a tear instead of a
  held frame), fast (no wait, no tear) or vrr (tearing allowed, capped 3 under the refresh). Evidence: scenarios
  `vsync_*` on this PC's 280 Hz monitor, uncapped: on 280 fps, adaptive 295, off 572, fast 559, vrr 277; no hitches.
- [x] **17. Window modes** - SDK: fullscreen, borderless, windowed, any resolution.

### Performance
- [ ] **19. Steady frame rate** - measure with the frame-time suite at 60 and above.
- [x] **20. Shader and pipeline cache** across runs - SDK: compiled shaders kept in `cache\shaders.bin`; the
  driver keeps its own pipeline cache. Evidence: recorded lobby run twice cold and twice warm
  (`turns\tests-20261116-055150`): worst frame 204-208 ms cold, 76-77 ms warm; 0.1% low 8 to 17-18 fps, 1% low 33 to
  41-43; hitches over 100 ms 2 to 0. The two left (53 and 77 ms) are the level entry (map open, shadow map made).
- [ ] **21. Precompile every shader** before play (from a recorded list).
- [~] **22. 30 to 360 fps at the right game speed** - SDK has an fps cap (60 by default); Whiplash's game logic speed
  above its own rate: the intro and the attract timer keep real time uncapped (scenarios `speed60` and `speed_uncapped`:
  the same frames at 8, 14, 20 s at 60 and ~560 fps; attract movie at 56 s and 53 s). Gameplay physics and cutscenes
  at high rates still to check before the default cap goes above 60.
- [~] **23. Graphics presets** - `graphics_preset` = custom, original, low, high, max (src/common/game_info.cpp).
  Evidence: `turns\tests-20261116-070924`: max renders at 7.95x with 4096 shadows, 60 fps, 1% low 52.8, 0.1% low
  48.4, no hitches; original gives the 4:3 console picture. The Advanced page comes with the Configuration app.
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
- [~] **34. Colour grading** - exposure, contrast, saturation, temperature, tint (`grade_*`) at present. Missing:
  LUT files, and the HUD keeping its colours (it is graded with the picture).
- [x] **37. Anisotropic filtering** - SDK: `aniso`; Unchained defaults to 16x (log: `anisotropic_filtering = 16`).
- [x] **38. Sharpening** - FSR 1 RCAS, `sharpen` 0-100 (0 off). Evidence: scenario `fsr` (sharpen 50).
- [x] **39. Higher-resolution shadows** - `shadow_resolution` (default 2048, 0 follows the render scale) through
  `GameInfo::target_scale`. Evidence: log `unchained: shadow map at 2048x2048`.
- [~] **40. Field of view** - `fov` (vertical degrees, 60 the game's) replaces only the game's 60 degree projection, so
  cameras with their own angle keep it. Not yet checked in play or in cutscenes.

### Player features
- [ ] **43. Settings in the game's own Options** - video, audio, controls pages.
- [x] **44. Direct start plus a Configuration app** - Unchained.exe starts the game when its files are there;
  `Unchained.exe --configure` (or a first run) opens the Configuration app (src/launcher, `GameInfo::launcher`).
- [ ] **45. One-click updates** from the Configuration app.
- [x] **46. Save profiles** - SDK `save_profile`: a named profile keeps its own disk (`profiles/<name>`, saves and
  their backups). Evidence: scenario `profile` - log "hard disk under ...profiles/test profile", partitions made.
- [x] **47. Save backups** at each start, the last 5 kept - SDK `save_backups` (5): E:\UDATA to
  `save backups\<date time>` before boot. Evidence: log `saves: backed up to ...\save backups\2026-11-16 051540`;
  three runs, three folders.
- [~] **48. Custom save folder** - SDK `save_folder` (the console's hard disk, saves included). Built, not yet run
  with a folder set.
- [ ] **50. Cheats overlay** - money, invincibility, unlocks. Not through the disc's Game.ini: its debug switches
  ("Player invincible", "Enable debug menus", "Disable Creature Shadows", "Hide All") are not in the retail xbe and
  are ignored (served rewritten, 2026-11-16: no change). Needs the game's own player and money fields.
- [ ] **51. Free camera and photo mode** - HUD hidden, a picture key.
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
- [ ] **9. Platforms** - Windows first; Linux needs the renderer ported off D3D12 (try Proton meanwhile).
- [x] **18. Widescreen and ultrawide** - Whiplash ignores the widescreen TV flag (the SDK's old 16:9 just stretched
  it). Its projection (`0x1390C0`) has a vertical fov, so `src/game/widescreen.cpp` widens the game's 4:3 aspect by
  the output's: `widescreen = "expand"` (default) fills 16:9 up to 32:9, 2D stays 4:3 in the middle, full screen
  passes stay full width; "off" is 4:3, "console" the old flag. The 448-line crop is `GameInfo::view_h`. Evidence:
  2026-11-16 run at 2544x1401, title and training level (`turns\multi-20261116-040301`, `shown.bmp`): wide 3D,
  HUD in the 4:3 middle, round icons round.
  Pop-in at the widened edges (user, 2026-11-16): the game culled with its 4:3 view; the camera's aspect getter
  (0x17F380) now gives the widened aspect, so the view, its culling and the projection agree. Evidence: the lab's
  first frames draw 539 and 520 times instead of 483 and 454; the domed doorway at the right edge, missing before, is
  drawn (turns/tests-20261116-071923 against turns/tests-20261116-071335).
- [ ] **30-32. Modern upscalers and frame generation** - FSR 3/4, DLSS need motion vectors a D3D8 game does not make;
  later, from depth and camera reprojection.
- [~] **33. Draw distance** - `draw_distance` (percent, 100-1000) raises the cameras' far clip in the projection
  hook (src/game/widescreen.cpp). Indoors (lab, 400%) nothing changes and depth stays right; needs an outdoor level
  to show, and the game's own distance culling and level-of-detail distances are not found yet.
- [ ] **35. HDR10** - tone-map the game's output; needs correct brightness first (gamma ramp, `0x2B5740`).
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
  player's own disc art at runtime) - Genron Corporation's lab look: dark teal, the green terminal
  HUD frames, the "FREEDOM / CRAZY / MADNESS" word style for headings, Spanx and Redmond as accents. No game art in
  the repo: our own drawing, colours and fonts; art from the player's own disc only at runtime after setup.
- [x] **L2. First-run flow** (pick, verify, copy, build the code with its progress in the app, then PLAY) - choose the disc image or folder, verify, extract to the data folder, recompile with a
  themed progress screen, then start straight into the game.
- [x] **L5. Installs as Savage Wind's do** (user, 2026-11-16) - the release zip holds the program only (exe, a
  `Unchained Configuration.exe` beside it, the bundled compiler and recompiler); the setup checks the player's files
  and copies them into the data folder; the game code is built once on their PC; saves, settings, logs and caches
  live in the data folder, the exe's by default, `UNCHAINED_DATA_ROOT` moves it (as `SAVAGEWIND_DATA_ROOT`). Done:
  the Configuration exe (release.bat; PLAY starts Unchained.exe), the data root (log "Data: ..."), the code build
  inside the setup with its progress in the app (`app::StartGameCodeBuild`). Evidence: a release-like folder (exe
  and compiler only): setup copied the files, built the code in 35 s ("The game's code is built"), and the next start
  went straight into the game without a build.
- [ ] **L3. Settings split** - quick ones in the game's Options, everything else in the Configuration app.
- [x] **L4. Project conventions** - no game files in the repo or builds, a weighted status page with progress bars
  (`tools/todo_progress.py` into STATUS.md), a [FAQ](FAQ.md), a bug report template
  (`.github/ISSUE_TEMPLATE/bug_report.md`), [SETTINGS.md](SETTINGS.md).

## Not applicable

7 (voice-bank crash fix), 13 (gyro), 42 (ambient occlusion and volumetric fog as new effects, not planned),
57 (achievements: no save-progress format known yet), 58 (amiibo).
