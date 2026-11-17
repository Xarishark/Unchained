# Release preparation

Where Unchained stands for its first public build, a preview: what has to happen before a zip goes out, what the
release notes must say, and every feature built from the first commit (Sun 15 Nov 2026, 04:13) to now. The PC
feature list itself is [TODO.md](TODO.md); progress is [STATUS.md](STATUS.md).

## Before the first public build

### Blockers

- [ ] **The runtime changes exist only on this PC.** The game repo's `sdk` submodule points at 42 commits on the
  local branch `whiplash-pr27`, pushed nowhere; a clone cannot build. Either push the branch to a fork the
  submodule can name, or land the general pieces upstream as pull requests (check upstream's development branch
  and open PRs first, so nothing is built twice), then point the submodule there.
- [ ] **The game repo has no remote.** Pick its host and name, and public or private.
- [x] **Licence notices.** `THIRD_PARTY.txt` (in the zip) now has AMD FSR 1, Zydis and Zycore beside toml++, Dear
  ImGui and SDL (SDK `47ad82e`); LLVM's licence ships beside clang.
- [x] **Unchained's own licence.** MIT (`LICENSE`), with a note that it covers Unchained's code only.
- [ ] **A clean-machine test of the zip.** `release.bat` on today's build, extracted to a new folder on a PC
  without Visual Studio: first run, setup, the code build, PLAY, a second start straight into the game. Last done
  at 08:03 on Mon 16 Nov, before the later features.
- [ ] **History to publish.** Three commit messages name another project (`808727f`, `8ab1831`, `be3919f`);
  decide whether to reword them before the history is public.

### Should do

- [x] **README** brought up to date, with a "Testers wanted" section and a not-affiliated disclaimer.
- [x] **Contribution rules** (`CONTRIBUTING.md`): no game files, moderation, `test` for development and `main` for
  releases, what a report needs; issue forms for bugs and features, and a pull request template.
- [~] **A version and a changelog**: `0.1.0-preview.1` (`src/common/version.h`) on the About page and the log's
  first line; still to do: the zip's name and a CHANGELOG.
- [ ] **FAQ**: Windows SmartScreen on an unsigned exe ("More info", "Run anyway"), and where crash dumps and logs
  are for a bug report.
- [ ] **Local paths in the docs**: STATUS.md and TODO.md name this PC's working folders (test tools, the reference
  video); fine for the team, odd in a public repo.
- [ ] **One playthrough past the first levels** before calling it more than a preview (TODO item 4).

### Release notes: known issues

- Spoken dialogue does not play yet (the sound effects and music do).
- Only the first levels are played through; later levels are untested.
- The mouse camera is off: keyboard and mouse work, but the mouse does not turn the camera yet.
- Above 60 fps is checked in the intro and menus only, so the cap stays at 60 by default.
- HDR10 is in but untested on an HDR display; colour LUTs and photo mode are early.
- A first start on a PC compiles shaders as it goes: short hitches at the first loading-screen cuts, gone from the
  second start.
- Windows only (Linux would need the renderer ported off Direct3D 12).

## What is done, from day 1

Times are the commits' (Sun 15 and Mon 16 Nov 2026). The runtime pieces are in the SDK (`sdk/runtime`), the
Whiplash pieces in `src/`.

### The port itself (Sun 15 Nov)

- **04:13** The project from the XboxXBERecomp game template: Whiplash's Xbox code recompiled to C++ (25,805
  functions); library functions named by hand where the signatures missed them.
- **17:41** Boots to presented frames, after seventeen boots naming the Direct3D and DirectSound pieces it needed;
  the double present that strobed the picture fixed.
- **19:03** A push buffer interpreter: Whiplash writes raw graphics commands, which the runtime now runs; the intro,
  the title text and the road render, with their texture addressing, filters, fog and depth.
- **19:35** Keyboard and scripted input; the disc check passes, so the hard disk mounts and saves work.
- **19:53** The characters render (their vertex data read as whole dwords).

### Rendering fixes (Mon 16 Nov, night)

- **01:00** Shadows render into their own texture (render targets and clears the game pushes itself).
- **01:01** Cutscene letterbox kept (draws stay inside the surface clip); **02:34** the intro's too.
- **01:14** Voice streams pause and resume (`CDirectSoundStream_Pause` mapped).
- **01:20-01:45** Hyper mode: its texture stage programs, a steady trail, the trail reading the frame being drawn.
- **03:04** The game's gamma ramp mapped; **02:46** its debug prints logged.
- **03:29** A sweep for library functions nobody mapped, stale graphics state and frame buffer reads.

### Picture

- **04:04** Rendering at the window's real resolution (render scale 3.975 at 2544x1401, never below the
  console's), supersampling past 100%; widescreen and ultrawide by widening the game's own projection (16:9 to
  32:9), 2D kept 4:3; 4:3 with bars and the console's stretch as options.
- **05:03** AMD FSR 1 upscaling and RCAS sharpening when rendering below the window's size; colour grading
  (exposure, contrast, saturation, temperature, tint); 2048 shadow maps instead of 256; 16x anisotropic filtering;
  a field of view setting.
- **07:18** Draw distance setting (the cameras' far clip).
- **07:21** No pop-in at the wide edges: the camera reports the widened view, so the game draws what the edges show.
- **07:11** Graphics presets: Original (the console's picture), Low, High, Max.
- **07:46** Character shadows toggle.
- **09:35-09:59** Clean menu and HUD edges: flat 2D art (the font, menu and dialog frames, box pieces, the HUD's
  green terminal icons) drawn 4x with sharp edges; painted art, gradients and static left alone.

### HUD and menus

- **06:59** HUD pieces at the screen's edges like a modern HUD, at any aspect, with their text inside; menus and
  centred text stay centred; `hud_edge` from the console's place to the edges. **07:05-07:08** pieces kept on
  screen; the press-start screen's Login selector kept on one line.
- **09:14-09:16** Button prompts for the device in use, switching live: today's Xbox pad, PlayStation symbols,
  Nintendo letters by position, keyboard key caps with the bound keys; the console's own glyphs as an option.

### Frame pacing and performance

- **05:14** VSync modes: off, on, adaptive, fast, VRR (capped 3 under the refresh); measured on a 280 Hz screen.
- **05:19** The game keeps real time with no cap (the intro and attract timer checked at 60 and ~560 fps).
- **05:56** Compiled shaders kept between runs: worst frame 204-208 ms on a first start, 76-77 ms after.
- **09:05** Every pipeline listed and made ahead at start; the hitches left on a warm start are the game's own
  level loads at loading-screen cuts.
- **07:57** Performance overlay (F10): fps, a frame time graph, 1% and 0.1% lows, hitches, the render size.

### Controls

- **05:19** Camera speed, inverted camera axes, vibration strength.
- **07:15** Every action rebindable (`[keys]`), keyboard and mouse buttons; the mouse camera is built but off for
  now (set aside, 18:33).
- Any controller through SDL3, read by button position.

### Saves and files

- **05:19** Save backups at each start (the last 5 kept); a custom save folder.
- **07:46** Save profiles: named sets of saves, each on its own disk.
- **07:23** Only the supported Xbox release starts; another is refused with both SHA-1s.
- **07:59** A movable data folder (`UNCHAINED_DATA_ROOT`) for saves, settings, logs and caches.

### The Configuration app and install

- **07:53** The Configuration app in the Genron lab look (dark teal, green terminal frames, the FREEDOM / CRAZY /
  MADNESS headings, Windows' own fonts, no game art): play, display, graphics, controls, sound and saves, game
  files, about. `Unchained.exe` starts the game directly once it is set up.
- **08:03** First run: pick the disc image or folder, the release checked, the files copied, the game code built on
  the player's PC with its progress on the same page, then PLAY. The zip holds the program only (the exe, the
  Configuration exe, the bundled compiler), no game code or files.

### Extras

- **08:41** F12 screenshots (PNG, the window's picture).
- **07:27** Accessibility: colour vision filters (protanopia, deuteranopia, tritanopia, with a strength) and a
  HUD-off switch.
- **07:39** Mods: texture packs and texture dumps; a hook for files served in place of the disc's.
- **08:45** Colour LUTs (.cube) and **08:54** HDR10 output (both early, set aside).
- English only: the disc has no other languages.

### Project and testing

- **05:03** An automated play-test suite: scripted or recorded input (the user's recorded lobby run), in-game
  captures, comparison with a reference video, per-present frame times with 1% and 0.1% lows, hitches and stutter.
- **07:40** Docs: settings, mods, FAQ, a bug report template, a progress table.
