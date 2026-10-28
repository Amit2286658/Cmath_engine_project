# Prebuilt binaries

Drop-in prebuilt engine binaries for Android apps. Apps that want to check
dynamically whether a newer engine build exists read `VERSION.txt` here.

| path | contents |
|---|---|
| `arm64-v8a/libmathengine.so` | Math Engine (MEP) shared library — Android bionic, API 24+, ARM aarch64. Exports both the `eng_*` C API (`Core/Headers/math_engine.h`) and the `Java_com_mathengine_MathEngineBridge_*` JNI surface (`Core/jni/`). |

## Rebuilding

```console
$ ANDROID_NDK_TOOLCHAIN_BIN=<ndk>/toolchains/llvm/prebuilt/<host>/bin \
      tools/build_android_arm64.sh
```

The script compiles the engine as PIE shared library with `-O2`, links with
lld, strips it, and writes `Prebuilt/arm64-v8a/libmathengine.so`.

## Version check contract

- `Prebuilt/VERSION.txt` contains a single line with the engine version
  (currently `1.0.0`, matching `project(cmath_engine VERSION 1.0.0)` in
  `CMakeLists.txt`).
- Bump that line whenever a new `.so` is shipped in this folder.
