# Unchained

**Unchained** is an unofficial native PC port of the original Xbox version of *Whiplash* (Crystal Dynamics / Eidos, 2003), made by static recompilation. **Work in progress:** it boots and plays its first levels with sound, saves and controls; the rest of the game is
being worked through. The build produces `Unchained.exe`.

What it adds to the console game: rendering at your screen's resolution (or supersampled), widescreen and ultrawide
with the HUD at the screen's edges, AMD FSR 1 and sharpening, higher-resolution shadows, 16x texture filtering, VSync
and VRR modes, graphics presets, colour grading and colour vision filters, rebindable keyboard and mouse with a mouse
camera, camera speed and vibration settings, save backups, texture packs, and an automated play-test suite.
See [docs/SETTINGS.md](docs/SETTINGS.md), [docs/MODS.md](docs/MODS.md) and [docs/FAQ.md](docs/FAQ.md).

This port is built with the [Xbox XBE Recomp](https://github.com/TGP482/XboxXBE-Recomp) tool.

> [!NOTE]
> This project does not include any game assets or game code. You must provide the files from your own legally acquired copy of the game to build or play.

## Playing

**You need:** your own copy of *Whiplash* for the original Xbox (USA): its disc image (`.iso`) or its extracted
files. Windows 10 or later, a GPU with Direct3D 12, and about 5 GB of free disk space.

1. Download the latest build and extract the zip to a folder you can write to, not Program Files.
2. Start `Unchained Configuration.exe` (or `Unchained.exe`, which opens it the first time). On **GAME FILES**, choose
   your disc image or folder. It checks your `default.xbe` is the supported release, copies the files beside
   Unchained, and builds the game's code once on your PC, with its progress on the same page. Your own files aren't
   changed.
3. Press **PLAY**. Later, `Unchained.exe` starts the game directly; the Configuration app has the settings.

Saves, settings, logs and caches stay in Unchained's folder (the data folder). `UNCHAINED_DATA_ROOT` moves it
elsewhere. Something wrong? See the [FAQ](docs/FAQ.md).

## Supported game version

Only **Whiplash (USA)** is supported, with the retail `default.xbe` from a Redump image:

| | MD5 |
|---|---|
| Retail `default.xbe` (supported; SHA-1 `3ea30323cec5734ddd4d535b5a269786b6900dc4`, which the setup checks) | `ec7857a3065b582ed2b3ca479e40531e` |
| xiso-tool patched copy (rejected) | `d0c5314a77f827c7b55b371c752a8d68`. One byte at file offset `0x123921` is `EB`; retail has `7D`. |

The data files are identical between the Redump and xiso images.

## Building

1. Clone with submodules: `git clone --recursive`.
2. Put the game's files in `assets/`: `default.xbe`, `Game.ini`, `Xbox.rkv`, `Xbox.r01`–`Xbox.r09`, `map/`, `movies/`, `objects/`, `newgame.dat`, `xbreverb.bin`.
3. Install Visual Studio 2022 with **Desktop development with C++**, **C++ Clang tools for Windows** and **C++ CMake tools for Windows**.
4. Run `build.bat` (from `cmd`, type `.\build.bat` if your system doesn't search the current folder for commands). It checks the XBE, writes `config/game.toml` with `xbe_symbols`, then recompiles and builds. The first build takes 30 to 90 minutes.

## Status

See [docs/STATUS.md](docs/STATUS.md).

## Credits

- [Xbox XBE Recomp](https://github.com/TGP482/XboxXBE-Recomp), the recompiler and runtime this is built on.
- [XenonRecomp](https://github.com/hedge-dev/XenonRecomp) for the overridable function layout and instruction hooks.
- [AMD FidelityFX Super Resolution 1](https://github.com/GPUOpen-Effects/FidelityFX-FSR) (MIT), in the SDK's `third_party/fsr1`.
- [Zydis](https://github.com/zyantific/zydis), [toml++](https://github.com/marzer/tomlplusplus), [Dear ImGui](https://github.com/ocornut/imgui) and [SDL](https://github.com/libsdl-org/SDL).
