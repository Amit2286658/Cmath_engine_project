#!/bin/sh
# Build Prebuilt/arm64-v8a/libmathengine.so (Android bionic, API 24).
#
# Usage:
#   build_android_arm64.sh [NDK_TOOLCHAIN_BIN_DIR]
# Default toolchain path: $ANDROID_NDK_HOME/toolchains/llvm/prebuilt/linux-*/bin
#
# Requires: CMakeLists.txt (sources list), NDK >= 27.
set -eu

NDK_BIN="${ANDROID_NDK_TOOLCHAIN_BIN:-}"
if [ -z "$NDK_BIN" ]; then
    for cand in "$ANDROID_NDK_HOME"/toolchains/llvm/prebuilt/*/bin \
                "$(dirname "$(command -v clang 2>/dev/null || true)")"; do
        if [ -x "$cand/clang" ]; then NDK_BIN="$cand"; break; fi
    done
fi
[ -n "$NDK_BIN" ] || { echo "error: cannot locate NDK toolchain bin (set ANDROID_NDK_TOOLCHAIN_BIN)" >&2; exit 1; }
echo "using NDK toolchain: $NDK_BIN"

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
API=24
ABI=arm64-v8a
OUT="$ROOT/Prebuilt/arm64-v8a"
TMP="$ROOT/Prebuilt/.build-$ABI"
ARCH=aarch64
CC="$NDK_BIN/${ARCH}-linux-android${API}-clang"
LD="$NDK_BIN/lld"

INCS="-I$ROOT/Core/Headers -I$ROOT/Core/compile -I$ROOT/Core/session -I$ROOT/Core/jni -I$ROOT/Core/lib/delegate -I$ROOT/Core/lib/hardcode"

CFLAGS="-O2 -std=c11 -fPIC -fvisibility=hidden -fstack-protector-strong \
 -DANDROID -DMATH_ENGINE_SHARED=1 -D_FORTIFY_SOURCE=2 -ffunction-sections"

rm -rf "$TMP"; mkdir -p "$TMP" "$OUT"

OBJS=""
i=0
for src in \
    "$ROOT/Core/Code/"*.c \
    "$ROOT/Core/compile/"*.c \
    "$ROOT/Core/session/"*.c \
    "$ROOT/Core/jni/"*.c \
    "$ROOT/Core/lib/delegate/"*.c \
    "$ROOT/Core/lib/hardcode/"*.c
do
    i=$((i+1))
    obj="$TMP/obj_$(printf '%02d' "$i")_$(basename "${src%.c}").o"
    "$CC" $CFLAGS $INCS -c "$src" -o "$obj"
    OBJS="$OBJS $obj"
    echo "cc  $(basename "$src")"
done

# Link directly with lld (avoids the clang driver's cc1 re-spawn path, which
# is problematic under qemu-user emulation hosts).
"$LD" -flavor gnu -m aarch64linux \
    -shared --sysroot="$NDK_BIN/../sysroot" \
    -soname libmathengine.so \
    -o "$TMP/libmathengine.so" \
    -L"$NDK_BIN/../sysroot/usr/lib/$ARCH-linux-android/$API" \
    -lm \
    $OBJS

"$NDK_BIN/llvm-strip" "$TMP/libmathengine.so"
mv "$TMP/libmathengine.so" "$OUT/libmathengine.so"
rm -rf "$TMP"
echo "built: $OUT/libmathengine.so"
