# Whiplash - XBE Recomp

An unofficial native PC port of the original Xbox version of *Whiplash* (Crystal Dynamics / Eidos, 2003), made by static recompilation. **Work in progress: it doesn't boot yet.**

This port is built with the [Xbox XBE Recomp](https://github.com/TGP482/XboxXBE-Recomp) tool.

> [!NOTE]
> This project does not include any game assets or game code. You must provide the files from your own legally acquired copy of the game to build or play.

## Supported game version

Only **Whiplash (USA)** is supported, with the retail `default.xbe` from a Redump image:

| | MD5 |
|---|---|
| Retail `default.xbe` (supported) | `ec7857a3065b582ed2b3ca479e40531e` |
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
- [Zydis](https://github.com/zyantific/zydis), [toml++](https://github.com/marzer/tomlplusplus), [Dear ImGui](https://github.com/ocornut/imgui) and [SDL](https://github.com/libsdl-org/SDL).
