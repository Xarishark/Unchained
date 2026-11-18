**Unchained 0.1.1**: *Whiplash* (original Xbox, 2003) as a native PC port for **Windows and Linux**, now ready for **[RecompLauncher](https://recomplauncher.com/game/unchained)**. Gameplay and the picture are the same as 0.1.

> **Bring your own copy.** No game files are included. You need your own *Whiplash* for the original Xbox (USA), as a disc image or its extracted files. The game's code is built on your PC from your copy the first time you start it.

## Downloads

| File | For |
| --- | --- |
| `Unchained-0.1.1-Windows-x64-Setup.exe` | Windows 10 / 11: the installer (for your user only, no admin) |
| `Unchained-0.1.1-Windows-x64.zip` | Windows: portable, extract and run |
| `Unchained-0.1.1-Linux-x86_64.AppImage` | Linux and Steam Deck: one file, everything inside |
| `SHA256SUMS.txt` | Checksums |

The file names now say their system, so launchers and players pick the right one. Unchained isn't code-signed yet, so Windows SmartScreen may warn on the first run (More info → Run anyway).

## What's new

- **RecompLauncher support.** Every release carries a `recomplauncher.json`: the launcher checks your copy against the known good dump, sets the game up with its own progress bar, starts it, and shows Unchained's settings (graphics, display, HUD, controls) on the game's page and in Big Picture mode, with your saves backed up and the logs one click away.
- **`--game <disc image or folder>`** plays your copy where it is, without copying it into Unchained's folder. Good for shortcuts, scripts and launchers.
- **`--prepare`** does the first start's setup (check the copy, build the game's code) without a window, printing `progress` lines.
- **Exit codes** a launcher can explain: 3 no copy found, 4 another release of the game, 5 the game's code couldn't be built.
- **Big Picture mode**: started from RecompLauncher's Big Picture mode, a windowed setting opens full screen for that run.
- **Portable AppImage**: an empty `portable.txt` next to the AppImage keeps saves, settings and logs in its folder instead of `~/.local/share/Unchained`.

## Known issues

The same as 0.1: some of the partner's spoken lines don't play yet, the lab's wall-lamp beams flicker at some distances, only the opening levels have been played through, and the frame cap stays at 60 by default. The Linux side of this release's new options is untested so far; please report anything odd.

*Unchained is a fan project, not affiliated with Crystal Dynamics, Eidos, Square Enix, Embracer Group or Microsoft. Made with AI: written by Claude (Anthropic) through Claude Code, directed and played by devall96.*
