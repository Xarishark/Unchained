**Unchained 0.1**: the first release of *Whiplash* (original Xbox, 2003) as a native PC port for **Windows and Linux**, made by static recompilation.

> **Bring your own copy.** No game files are included. You need your own *Whiplash* for the original Xbox (USA), as a disc image or its extracted files. The game's code is built on your PC from your copy the first time you start it.

## Downloads

| File | For | Size |
| --- | --- | --- |
| `Unchained-0.1-Setup.exe` | Windows 10 / 11: the installer (for your user only, no admin) | 55 MB |
| `Unchained-0.1.zip` | Windows: portable, extract and run | 83 MB |
| `Unchained-0.1-x86_64.AppImage` | Linux and Steam Deck: one file, everything inside | 148 MB |
| `SHA256SUMS.txt` | Checksums for all three | |

**Windows** needs a GPU with Vulkan 1.3 or Direct3D 12. **Linux** needs a distro from about 2022 on (Ubuntu 22.04, Debian 12, Fedora 36, SteamOS 3 or newer) and a Vulkan 1.3 driver. Nothing else to install on either: the compiler that builds the game's code comes with Unchained. Unchained isn't code-signed yet, so Windows SmartScreen may warn on the first run (More info → Run anyway).

Setup and the full feature list: [the project page](https://devall96.github.io/Unchained/) and the [README](https://github.com/devall96/Unchained#readme).

## What's in 0.1

- **The game as native code**: 25,805 functions, rebuilt on your PC at the first start. No emulator.
- **Your screen's resolution** and supersampling; **real widescreen and ultrawide** (16:9 up to 32:9) that shows more instead of stretching.
- **AMD FSR 1** with sharpening, 16x filtering, sharper shadows, field of view, draw distance, colour grading, clean menu and HUD edges.
- **Graphics presets**, including **OG Graphics**: the console as it was, 640x480 at 4:3.
- **The HUD at the screen's edges**, and **button prompts for your controller** (Xbox, PlayStation, Nintendo, or the console's own).
- **Vulkan by default** on both systems, Direct3D 12 as the fallback on Windows; shaders and pipelines kept and made ahead, so later starts don't stutter.
- **An in-game menu (F1 or Back + Start)** with every setting, a **performance overlay (F10)** and **screenshots (F12)**.
- **A Configuration app** that sets the game up from your disc image or folder.
- Save backups at every start, save profiles, colour vision filters and texture packs.

## Known issues

- Many of the partner character's spoken lines don't play yet (sound effects, music and the main character's voice do).
- The lab's wall-lamp light beams flicker at some distances; hyper mode's trail effect doesn't show yet.
- Only the opening levels have been played through on Unchained.
- Controller only for now; keyboard and mouse play is on the roadmap.
- The frame cap stays at 60 until gameplay above 60 is checked; draw distance doesn't reach the gameplay camera in the wide view yet.
- The title screen in widescreen shows a white strip at each side.
- The very first start compiles shaders as it goes, so the first loading cuts hitch once.

## Testers wanted

0.1 has been tested on one AMD PC, on Windows 11 and Ubuntu. Reports from **NVIDIA and Intel graphics, Steam Deck and other handhelds, laptops, other Linux distros, other controllers and ultrawide screens** would help a lot, including "it worked fine". Please use the [bug report form](https://github.com/devall96/Unchained/issues/new/choose) and attach the newest log from the `logs` folder.

*Unchained is a fan project, not affiliated with Crystal Dynamics, Eidos, Square Enix, Embracer Group or Microsoft. Made with AI: written by Claude (Anthropic) through Claude Code, directed and played by devall96.*
