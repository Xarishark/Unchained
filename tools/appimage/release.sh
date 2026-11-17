#!/bin/sh
# The Linux release, as release.bat is Windows': the game built in an Ubuntu 22.04 container (linux-portable), then
# packed with LLVM's own clang and lld into one AppImage.
#
#   tools/appimage/release.sh <image> <LLVM release dir> <runtime> [out.AppImage]
#
# <image> is a podman image of Ubuntu 22.04 with clang, lld, cmake 3.28+, ninja, python3 and SDL3's build libraries
# (X11, Wayland, PipeWire, PulseAudio, ALSA, libdecor, udev, dbus, ibus); <LLVM release dir> an unpacked
# LLVM-<version>-Linux-X64 (github.com/llvm/llvm-project/releases); <runtime> the AppImage type 2 runtime. The game
# files are read through assets/ as for any build, and nothing of them goes into the image.
set -e
image=$1
llvm=$(cd "$2" && pwd)
runtime=$(cd "$(dirname "$3")" && pwd)/$(basename "$3")
out=${4:-Unchained-x86_64.AppImage}
src=$(cd "$(dirname "$0")/../.." && pwd)
assets=$(readlink -f "$src/assets")

# the source at /src, the game files where assets/ points (read only)
podman run --rm --userns=keep-id -v "$src:/src" -v "$assets:$assets:ro" -w /src "$image" \
    sh -c 'cmake --preset linux-portable && cmake --build out/build/linux-portable'
LLVM_DIR=$llvm sh "$src/tools/appimage/build_appimage.sh" "$src/out/build/linux-portable" "$runtime" "$out"
