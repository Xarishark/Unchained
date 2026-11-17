# Contributing to Unchained

Thanks for wanting to help! Testing on different hardware, bug reports and code contributions are all welcome, and
playing through the game and reporting what breaks is the most useful thing anyone can do right now.

> [!NOTE]
> This project is developed mostly with AI (see the README). Contributions written by hand, with AI assistance or
> anything in between are equally welcome: what matters is that a change works, is tested, and follows the rules
> below.

## The one hard rule: no game files

Nothing from the game may ever be committed or attached to an issue or pull request:

- no copies of the game in any form: no disc images, no `default.xbe`, no `Xbox.rkv` or other data files
- no generated code (`generated/`): the game's code is only ever recompiled on the player's own PC
- no game assets of any kind: textures, models, sounds, music, movies, text, level files, also not "just one file
  for testing"; this includes the texture dumps Unchained can write
- no links to any of these, and no instructions for getting them other than from your own copy
- no code, headers or documentation copied from decompilations or leaked sources: Unchained's code is written from
  what we find in our own copy of the game

Pull requests that include game files can't be merged and will be closed. Screenshots and short clips of the game in
bug reports are fine, and so are your own save files, zipped and attached to an issue: a problem with the game's
mechanics can often only be followed from the save. Saves are for issues: don't commit them.

## Moderation

Bugs, requests and code all go through GitHub. Unchained is one person's project with the help of contributors, and
every contribution is reviewed and tested by the maintainer before it goes into `test`.

Issues, pull requests and comments that share, link to or ask for copies of the game or its files are removed. An
account that does it again is blocked.

Issues are for bug reports and feature requests. Don't open an issue to share code, patches or changed files: open a
pull request against `test` instead (see [Pull requests](#pull-requests)), so the change can be reviewed, tested and
credited to you. Issues that share code are closed with a pointer here.

## Which branch to work on

- **`test`** is where development happens: every change that is done and being tested, ahead of the latest release.
- **`main`** only moves for a release. Once `test` has been tested well, it goes into `main` and a release is
  published from it.
- **Base your work on `test` and open pull requests against `test`.** Before you start, look at
  [docs/TODO.md](docs/TODO.md) and [docs/STATUS.md](docs/STATUS.md) on `test`: what is done, in progress and
  planned, so nothing is built twice.

## Testing and reporting bugs

Use the bug report form. Every report needs:

- your Windows version, GPU and driver, CPU, and the window size
- the Unchained version (shown in Unchained Configuration's About page), or the `test` commit you built
- where in the game it happened (level and place, menu or cutscene), what you did, what happened and what you
  expected
- a screenshot (F12 saves one to `screenshots\`) or a short clip if it's visual
- the newest `logs\unchained_NNN.log` from Unchained's folder, and for a crash the newest file in `CrashDumps\`

Only **Whiplash (USA)** for the original Xbox is supported, from a retail disc image or its extracted files: the
setup checks `default.xbe` and refuses any other release.

Especially welcome: a playthrough of a level you haven't seen reported yet, different GPUs (AMD, Intel, older
NVIDIA), controllers other than an Xbox pad, ultrawide screens, and anything that stutters (the F10 overlay shows
frame times, lows and hitches).

## Building

See the README's [Building](README.md#building) section. In short: Visual Studio 2022 with the C++, Clang and CMake
workloads, your own game files in `assets/`, then `build.bat`. The first build recompiles the game and takes 30 to 90
minutes.

## Project layout

- `src/game/` - hooks on the game's own code (widescreen, the projection and the camera)
- `src/common/` - settings, controls, keyboard, button prompts, the overlay
- `src/launcher/` - the Configuration app and its first-run setup
- `sdk/` - the XboxXBERecomp recompiler and runtime (a submodule): Direct3D 8 to 12, the push buffer interpreter,
  the present pass, input, sound, saves; changes that help every game belong here
- `config/game.toml` - the recompiler's input: symbol addresses for this release, named by hand where needed
- `tools/tests/` - the automated play tests (`run_tests.py`, `scenarios.toml`): scripted or recorded input,
  in-game captures, per-frame timings
- `docs/` - status, the feature list, settings, mods, FAQ, release notes

## Testing your change

- Build cleanly with `build.bat`. If you touch rendering, say which GPU and driver you tested on.
- Test in the game and look at the result: a fix isn't verified until you've seen it work. Where a scenario in
  `tools/tests/scenarios.toml` covers it, run it (`py -3 tools/tests/run_tests.py <scenario>`).
- Performance changes are measured with per-frame timings (the tests' `frametimes.csv`: 1% and 0.1% lows, hitches),
  not an fps average, and must not make the lows or hitches worse.

## Code style

Match the surrounding code: its naming, structure and comment density. Comments explain *why* (what the game does,
what an address is, which bug showed the problem), not what the next line does.

## Pull requests

- Open it against `test`, on top of its latest commit (see "Which branch to work on").
- Keep each pull request focused on one change; small is easier to review.
- Describe what changed, why, and how you tested it.
- Don't include personal paths or information in code, comments or commit messages.

## License

By contributing, you agree that your contributions are licensed under the project's [MIT License](LICENSE).
