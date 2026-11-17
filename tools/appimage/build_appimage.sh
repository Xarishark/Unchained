#!/bin/sh
# Unchained-x86_64.AppImage from a Linux release build: one file, as the Windows release is one installer.
#
#   [LLVM_DIR=<LLVM release>] tools/appimage/build_appimage.sh <build dir> <runtime> <out.AppImage>
#
# Releases: <build dir> from the linux-portable preset in an Ubuntu 22.04 container (glibc 2.35, libstdc++ linked
# in; libgcc_s stays the system's, the game code's exception handling unwinds through it), LLVM_DIR an official LLVM
# Linux X64 release (github.com/llvm/llvm-project/releases): then the image needs glibc 2.35 and GLIBCXX_3.4.30
# (clang's), Ubuntu 22.04, Debian 12, Fedora 36, SteamOS 3 and newer. It mounts with the system's fusermount3 (fuse3,
# on every current desktop); without it, APPIMAGE_EXTRACT_AND_RUN=1 runs it unpacked.
#
# <build dir> is a linux-release build (Unchained, compiler/); <runtime> is the AppImage type 2 runtime
# (github.com/AppImage/type2-runtime, runtime-x86_64), which the squashfs is appended to. Inside:
#   usr/bin/Unchained, usr/bin/compiler/ (xbe_recomp, the configs and headers, clang and ld.lld for the first-start
#   build), usr/lib/ (SDL3 and the libraries clang and lld need), AppRun, the desktop entry and the icon.
# Left to the system, as AppImages leave them: glibc, libstdc++ and libgcc_s, the Vulkan loader and the GPU driver,
# libedit and libtinfo.
# The player's data goes to ~/.local/share/Unchained (app/host.cpp), the built game code to its gamecode/.
# Needs mksquashfs (squashfs-tools); nothing of the game is in the image.
set -e
build=$(cd "$1" && pwd)
runtime=$2
out=$3
here=$(cd "$(dirname "$0")" && pwd)
work=$(mktemp -d)
app=$work/Unchained.AppDir
trap 'rm -rf "$work"' EXIT

mkdir -p "$app/usr/bin/compiler" "$app/usr/lib"
cp "$build/Unchained" "$app/usr/bin/"
# the compiler folder without what a build left in it
(cd "$build/compiler" && tar cf - --exclude=./work --exclude=./build.lock .) | (cd "$app/usr/bin/compiler" && tar xf -)
# clang and ld.lld for the first-start build: LLVM's own Linux release where LLVM_DIR names one (what releases ship:
# built for old systems, glibc 2.34), else the build machine's
if [ -n "$LLVM_DIR" ]; then
    cp "$(ls "$LLVM_DIR"/bin/clang-[0-9]* | head -1)" "$app/usr/bin/compiler/clang"
    cp "$LLVM_DIR/bin/lld" "$app/usr/bin/compiler/ld.lld"  # lld picks its linker by the name it runs as
else
    cp "$(readlink -f "$(command -v clang)")" "$app/usr/bin/compiler/clang"
    cp "$(readlink -f "$(command -v ld.lld)")" "$app/usr/bin/compiler/ld.lld"
fi
cp "$here/../../sdk/tools/LLVM-LICENSE.txt" "$app/usr/bin/compiler/LLVM-LICENSE.txt"

# the libraries the three need, but the ones every desktop has and must use its own of. libedit and libtinfo (LLVM's
# line editing, which clang never uses here) are the system's: a build machine's copies can want a newer glibc than
# the rest (Ubuntu 26.04's want 2.42 for four terminal calls), and every desktop has libtinfo6 and libedit2
skip='^(libc|libm|libdl|libpthread|librt|ld-linux-x86-64|libstdc\+\+|libgcc_s|libvulkan|libGL|libEGL|libGLX|libdrm|libgbm|libwayland-.*|libX.*|libxcb.*|libasound|libpulse.*|libdbus-1|libsystemd|libudev|libedit|libtinfo)\.so'
for bin in "$app/usr/bin/Unchained" "$app/usr/bin/compiler/clang" "$app/usr/bin/compiler/ld.lld"; do
    ldd "$bin" | awk '/=> \// {print $3}' | while read -r lib; do
        name=$(basename "$lib")
        if ! echo "$name" | grep -Eq "$skip" && [ ! -e "$app/usr/lib/$name" ]; then
            cp -L "$lib" "$app/usr/lib/$name"
        fi
    done
done

cp "$here/AppRun" "$app/AppRun"
chmod +x "$app/AppRun"
cp "$here/unchained.desktop" "$app/unchained.desktop"
cp "$here/unchained.png" "$app/unchained.png"
ln -s unchained.png "$app/.DirIcon"

mksquashfs "$app" "$work/app.squashfs" -root-owned -noappend -comp zstd -quiet
cat "$runtime" "$work/app.squashfs" > "$out"
chmod +x "$out"
echo "$out: $(du -h "$out" | cut -f1)"
