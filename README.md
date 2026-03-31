# SZ-0C

High-fidelity Tempest (1981) style source port in C++17 using Raylib 5.5, with modern post-processing, procedural SFX, and embedded BGM/shader assets.

This repository is tuned for Windows + MSYS2 UCRT64 and currently targets a single self-contained Windows executable build.

## Highlights

- C++17 game code with modular systems (state, rendering, enemies, input, audio, persistence)
- Multi-pass bloom pipeline and enhanced VFX path
- Procedural SFX and libopenmpt music playback
- Shader and tracker-module assets embedded into the executable at build time
- Embedded Windows application icon used for Explorer, title bar, taskbar, and Alt+Tab
- Static-link focused packaging to minimize runtime dependencies

## Runtime Controls

- Move left/right: Left Arrow / Right Arrow
- Fire: Space
- Coin: 5
- Start: 1
- Superzapper: X
- Gamepad movement: Left stick X axis
- Gamepad fire: Right face down
- Gamepad superzapper: Right face right

## Requirements

- Windows 10/11 x64
- MSYS2 UCRT64 toolchain installed at C:/msys64/ucrt64
- Python 3 available on PATH (used by asset embedding script)
- CMake 3.25+
- Ninja

Install core packages from an MSYS2 UCRT64 shell:

```bash
pacman -Syu
pacman -S --needed \
  mingw-w64-ucrt-x86_64-clang \
  mingw-w64-ucrt-x86_64-cmake \
  mingw-w64-ucrt-x86_64-ninja \
  mingw-w64-ucrt-x86_64-raylib \
  mingw-w64-ucrt-x86_64-libopenmpt \
  mingw-w64-ucrt-x86_64-pkgconf
```

## Build Raylib (Bundled GLFW Static)

The project expects a custom static Raylib at:

- C:/dev/raylib-5.5/build-static/raylib/libraylib.a

Reason: MSYS2 prebuilt Raylib may still pull glfw3.dll. This custom build removes that dependency.

```powershell
cd C:\dev
# Place raylib-5.5 source at C:\dev\raylib-5.5 first
cmake -B C:\dev\raylib-5.5\build-static -G Ninja \
  -S C:\dev\raylib-5.5 \
  -DBUILD_SHARED_LIBS=OFF \
  -DUSE_EXTERNAL_GLFW=OFF \
  -DCMAKE_C_COMPILER=C:/msys64/ucrt64/bin/gcc.exe
cmake --build C:\dev\raylib-5.5\build-static --config Release
```

## Build SZ-0C

From repository root:

```powershell
cmake -B build -G Ninja -S .
cmake --build build
```

Output executable:

- build/sz.exe

## Run

Normal run:

```powershell
.\build\sz.exe
```

IRP scripted capture mode:

```powershell
.\build\sz.exe --irp
```

Notes:

- High scores are stored in sz_hiscore.bin in the current working directory.
- Boot diagnostics are written to sz_launch.log.

## Embedded Asset Pipeline

At build time, CMake invokes:

- cmake/gen_embedded_assets.py

It converts:

- src/shaders/*
- bgm_mod/* (MOD/XM/S3M/IT)

into generated C++ files under build/generated and compiles them into sz.exe.

## Project Structure

- src: game source code
- src/shaders: GLSL shader sources
- bgm_mod: tracker music source assets
- cmake/gen_embedded_assets.py: code generator for embedded assets
- cmake/toolchain-ucrt64-clang.cmake: MSYS2 UCRT64 toolchain definition
- CMakeLists.txt: build configuration and static-link strategy

## Distribution Notes

The build is optimized toward a single-file Windows deployment. Remaining imports should be Windows system runtime components.

If you move toolchains or Raylib source locations, update paths in CMakeLists.txt accordingly.

## Troubleshooting

- Error: Bundled-GLFW raylib not found
  - Build Raylib as shown above or update _RAYLIB_BUNDLED_STATIC in CMakeLists.txt.
- Link errors around static libs
  - Ensure MSYS2 UCRT64 libs are installed and compilers resolve to C:/msys64/ucrt64.
- Asset generation failures
  - Confirm Python is on PATH and source folders src/shaders and bgm_mod exist.

## License

This project is licensed under the MIT License.

See [LICENSE](LICENSE) for the full text.
