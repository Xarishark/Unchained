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
- [ ] **2. Game files setup** - first run asks for the disc image (.iso) or extracted folder, checks the XBE SHA-1,
  copies to the data folder, recompiles once with a progress screen. SDK: ISO mount and the first-launch code build
  exist; the setup screen and copy step do not.
- [~] **4. Whole game finishable** - training level plays; the rest untested. Track a playthrough per level.
- [~] **5. Sound and voices correct** - sound bank plays; spoken dialogue (streamed `vo/VO_*`) never plays
  (docs/STATUS.md).
- [ ] **8. One supported version** - refuse an XBE whose SHA-1 is not `3ea30323cec5734ddd4d535b5a269786b6900dc4`,
  with a clear message.

### Platforms and input
- [~] **10. Any SDL controller** - SDL3 pad to XInput HLE is up; confirm by button position, every pad open, the last
  one used is read.
- [ ] **11. Camera speed and vibration strength** settings (the user found the camera "weird and off").
- [~] **12. Keyboard and mouse** - fixed keys exist (`src/common/input_keys.cpp`); add rebinding and a real mouse
  camera.
- [ ] **14. Handheld preset** - detect PC handhelds; 30/40/45/60 fps and the screen's own size.
- [ ] **15. Button prompts for the device in use** - Xbox, PlayStation or keyboard glyphs in the game's prompts,
  switching live.
- [~] **16. VRR and VSync modes** - SDK has VSync on/off; add Fast / Adaptive and VRR-friendly presentation.
- [x] **17. Window modes** - SDK: fullscreen, borderless, windowed, any resolution.

### Performance
- [ ] **19. Steady frame rate** - measure with the frame-time suite at 60 and above.
- [ ] **20. Shader and pipeline cache** across runs.
- [ ] **21. Precompile every shader** before play (from a recorded list).
- [~] **22. 30 to 360 fps at the right game speed** - SDK has an fps cap; Whiplash's game logic speed above its
  own rate is untested (menus, cutscenes, physics).
- [ ] **23. Graphics presets** - Original, High, Max (and a low one) plus an Advanced page.
- [ ] **25. Faster release build** - profile-guided optimisation and ThinLTO for the runtime.
- [ ] **26. Performance overlay** - fps, frame time graph, GPU use.
- [ ] **27. Benchmark routes** - scripted runs (`UNCHAINED_INPUT`) that write per-present frame times.

### Graphics
- [x] **28. Resolution and render scale** - SDK: `render_percent`. Fixed 2026-11-16: the scale followed the
  `width`/`height` setting, so a 640x480 setting in a 2544x1401 window rendered at 1.0 and was upscaled; it now
  follows the window (log: `render scale 3.975`), never under the console's, 200+ supersamples.
- [ ] **29. FSR 1** upscaling.
- [ ] **34. Colour grading** - exposure, contrast, saturation, temperature, tint, LUTs; HUD keeps its colours.
- [x] **37. Anisotropic filtering** - SDK: `aniso`.
- [ ] **38. Sharpening** (AMD CAS).
- [ ] **39. Higher-resolution shadows** - the game's 256x256 shadow target (pushed surface at 0xD3E980) drawn at
  1024 or 2048.
- [ ] **40. Field of view** setting; cutscenes keep theirs.

### Player features
- [ ] **43. Settings in the game's own Options** - video, audio, controls pages.
- [ ] **44. Direct start plus a Configuration app** (see Launcher below).
- [ ] **45. One-click updates** from the Configuration app.
- [ ] **46. Save profiles.**
- [ ] **47. Save backups** at each start, the last 5 kept.
- [ ] **48. Custom save folder.**
- [ ] **50. Cheats overlay** - money, invincibility, unlocks.
- [ ] **51. Free camera and photo mode** - HUD hidden, a picture key.
- [ ] **54. Translated menus** for Unchained's own screens.
- [ ] **55. Discord Rich Presence**, off by default.
- [ ] **56. Steam Deck and Add to Steam.**

## Adapted to Whiplash

- [ ] **1. The whole game from your own disc** - one game, no DLC; ISO or extracted folder.
- [ ] **6. Languages** - whatever text and voice languages the disc has, chosen in settings.
- [ ] **9. Platforms** - Windows first; Linux needs the renderer ported off D3D12 (try Proton meanwhile).
- [x] **18. Widescreen and ultrawide** - Whiplash ignores the widescreen TV flag (the SDK's old 16:9 just stretched
  it). Its projection (`0x1390C0`) has a vertical fov, so `src/game/widescreen.cpp` widens the game's 4:3 aspect by
  the output's: `widescreen = "expand"` (default) fills 16:9 up to 32:9, 2D stays 4:3 in the middle, full screen
  passes stay full width; "off" is 4:3, "console" the old flag. The 448-line crop is `GameInfo::view_h`. Evidence:
  2026-11-16 run at 2544x1401, title and training level (`turns\multi-20261116-040301`, `shown.bmp`): wide 3D,
  HUD in the 4:3 middle, round icons round.
- [ ] **30-32. Modern upscalers and frame generation** - FSR 3/4, DLSS need motion vectors a D3D8 game does not make;
  later, from depth and camera reprojection.
- [ ] **33. Draw distance** - find the game's far clip and level-of-detail distances and raise them.
- [ ] **35. HDR10** - tone-map the game's output; needs correct brightness first (gamma ramp, `0x2B5740`).
- [ ] **36. Effect toggles** - Whiplash's own: hyper-mode trail, fog, shadows, glow, screen effects.
- [ ] **41. Highest level of detail** at any distance.
- [ ] **49. Accessibility** - colour vision filters, invincibility, damage scaling, text speed, HUD off.
- [ ] **52. Level and area select** in place of a teleport.
- [~] **53. Mods** - SDK: texture replacement (`textures\replacement\`) and dumping; document it, add a mods folder.

## Launcher, files and theme (as SuperWonders did)

- [ ] **L1. Configuration app themed to Whiplash** - Genron Corporation's lab look: dark teal, the green terminal
  HUD frames, the "FREEDOM / CRAZY / MADNESS" word style for headings, Spanx and Redmond as accents. No game art in
  the repo: our own drawing, colours and fonts; art from the player's own disc only at runtime after setup.
- [ ] **L2. First-run flow** - choose the disc image or folder, verify, extract to the data folder, recompile with a
  themed progress screen, then start straight into the game.
- [ ] **L3. Settings split** - quick ones in the game's Options, everything else in the Configuration app.
- [ ] **L4. Project conventions** - no game files in the repo or builds, a weighted status page with progress bars,
  a FAQ, a bug report template.

## Not applicable

7 (voice-bank crash fix), 13 (gyro), 42 (ambient occlusion and volumetric fog as new effects, not planned),
57 (achievements: no save-progress format known yet), 58 (amiibo).
