#!/bin/sh
# Unchained-x86_64.AppImage from a Linux release build: one file, as the Windows release is one installer.
#
#   tools/appimage/build_appimage.sh <build dir> <runtime> <out.AppImage>
#
# <build dir> is a linux-release build (Unchained, compiler/); <runtime> is the AppImage type 2 runtime
# (github.com/AppImage/type2-runtime, runtime-x86_64), which the squashfs is appended to. Inside:
#   usr/bin/Unchained, usr/bin/compiler/ (xbe_recomp, the configs and headers, clang and ld.lld for the first-start
#   build), usr/lib/ (SDL3 and the libraries clang and lld need), AppRun, the desktop entry and the icon.
# Left to the system, as AppImages leave them: glibc, libstdc++ and libgcc_s, the Vulkan loader and the GPU driver.
# The player's data goes to ~/.local/share/Unchained (app/host.cpp), the built game code to its gamecode/.
# Needs mksquashfs (squashfs-tools) and the system's clang and lld; nothing of the game is in the image.
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
cp "$(readlink -f "$(command -v clang)")" "$app/usr/bin/compiler/clang"
cp "$(readlink -f "$(command -v ld.lld)")" "$app/usr/bin/compiler/ld.lld"
cp "$(dirname "$(readlink -f "$(command -v clang)")")/../../../share/doc/llvm-21/copyright" \
    "$app/usr/bin/compiler/LLVM-LICENSE.txt" 2>/dev/null || true

# the libraries the three need, but the ones every desktop has and must use its own of
skip='^(libc|libm|libdl|libpthread|librt|ld-linux-x86-64|libstdc\+\+|libgcc_s|libvulkan|libGL|libEGL|libGLX|libdrm|libgbm|libwayland-.*|libX.*|libxcb.*|libasound|libpulse.*|libdbus-1|libsystemd|libudev)\.so'
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
