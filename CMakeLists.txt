cmake_minimum_required(VERSION 3.25)
project(sz C CXX RC)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ── Compiler flags ──────────────────────────────────────────────────────────
add_compile_options(
    -O2
    -Wall -Wextra -Wpedantic
    -ffast-math
    -march=native
    # Suppress known Raylib/libopenmpt pedantic warnings
    -Wno-missing-field-initializers
    -Wno-unused-parameter
)

# ── Find Raylib (static, bundled GLFW) ──────────────────────────────────────
# Use a custom-built raylib that embeds GLFW statically (no glfw3.dll).
# The MSYS2 pre-built libraylib.a was compiled with USE_EXTERNAL_GLFW=ON
# and still imports glfw3.dll at runtime. Our build at C:/dev/raylib-5.5
# uses the default (bundled) GLFW, giving a truly standalone static lib.
set(raylib_USE_STATIC_LIBS ON)
set(_RAYLIB_BUNDLED_STATIC "C:/dev/raylib-5.5/build-static/raylib/libraylib.a")
if(NOT EXISTS "${_RAYLIB_BUNDLED_STATIC}")
    message(FATAL_ERROR "Bundled-GLFW raylib not found at ${_RAYLIB_BUNDLED_STATIC}. "
        "Build it with: cmake -B C:/dev/raylib-5.5/build-static -DBUILD_SHARED_LIBS=OFF "
        "-DUSE_EXTERNAL_GLFW=OFF C:/dev/raylib-5.5 && cmake --build C:/dev/raylib-5.5/build-static")
endif()

find_package(raylib REQUIRED
    HINTS "${CMAKE_SYSROOT}/lib/cmake/raylib"
          "C:/msys64/ucrt64/lib/cmake/raylib"
          "C:/tools/msys64/ucrt64/lib/cmake/raylib"
          "C:/tools/msys64/clang64/lib/cmake/raylib")

# ── Static audio/openmpt libraries ───────────────────────────────────────────
# Derive the lib directory from the compiler location so the correct
# MSYS2 toolchain (ucrt64, clang64, …) is always picked.
get_filename_component(_CXX_BIN_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)
get_filename_component(_TOOLCHAIN_ROOT "${_CXX_BIN_DIR}" DIRECTORY)
set(_LIB_HINTS "${_TOOLCHAIN_ROOT}/lib"
               "C:/msys64/ucrt64/lib"
               "C:/tools/msys64/ucrt64/lib"
               "C:/tools/msys64/clang64/lib")

foreach(_lib openmpt mpg123 sndfile FLAC vorbisfile vorbis ogg z winpthread)
    string(TOUPPER "${_lib}" _ulib)
    find_library(${_ulib}_STATIC "lib${_lib}.a" HINTS ${_LIB_HINTS} NO_DEFAULT_PATH)
    if(NOT ${_ulib}_STATIC)
        message(FATAL_ERROR "Static library lib${_lib}.a not found")
    endif()
endforeach()

# ── Embed shaders & BGM assets into the executable ───────────────────────────
set(_GEN_DIR "${CMAKE_BINARY_DIR}/generated")
set(_GEN_SCRIPT "${CMAKE_SOURCE_DIR}/cmake/gen_embedded_assets.py")

file(GLOB _ASSET_DEPS
    "${CMAKE_SOURCE_DIR}/src/shaders/*.fs"
    "${CMAKE_SOURCE_DIR}/src/shaders/*.vs"
    "${CMAKE_SOURCE_DIR}/bgm_mod/*.MOD"
    "${CMAKE_SOURCE_DIR}/bgm_mod/*.mod"
)

add_custom_command(
    OUTPUT  "${_GEN_DIR}/embedded_assets.hpp"
            "${_GEN_DIR}/embedded_assets.cpp"
    COMMAND python "${_GEN_SCRIPT}"
                   "${_GEN_DIR}"
                   "${CMAKE_SOURCE_DIR}/src/shaders"
                   "${CMAKE_SOURCE_DIR}/bgm_mod"
    DEPENDS ${_ASSET_DEPS} "${_GEN_SCRIPT}"
    COMMENT "Embedding shaders and BGM assets"
)

# ── Sources ─────────────────────────────────────────────────────────────────
file(GLOB_RECURSE SZ_SOURCES CONFIGURE_DEPENDS src/*.cpp)
# winpthread_stubs.c provides static __imp_pthread_xxx pointers so the
# GCC/libstdc++ runtime doesn't require libwinpthread-1.dll at runtime.
# sz_resources.rc embeds the application icon into the PE binary.
list(APPEND SZ_SOURCES
    "${CMAKE_SOURCE_DIR}/src/winpthread_stubs.c"
    "${CMAKE_SOURCE_DIR}/src/sz_resources.rc"
    "${_GEN_DIR}/embedded_assets.cpp"
)

# WIN32 hides the console window; icon is supplied via sz_resources.rc.
add_executable(sz WIN32 ${SZ_SOURCES})

target_compile_definitions(sz PRIVATE SZ_ENHANCED_VFX=1)

target_include_directories(sz PRIVATE
    src/
    "${_GEN_DIR}"
    # openmpt headers — derive from same toolchain root
    "${_TOOLCHAIN_ROOT}/include"
)

# ── Static linking: all third-party libs + runtime bundled into the exe ───────
# Use explicit .a paths for all third-party libs so the linker always
# chooses the static archive regardless of -Wl,-Bstatic ordering rules.
target_link_options(sz PRIVATE
    -static-libstdc++
    -static-libgcc
)

target_link_libraries(sz PRIVATE
    # All third-party static archives given as explicit paths.
    # Use the bundled-GLFW raylib so no glfw3.dll is needed.
    "${_RAYLIB_BUNDLED_STATIC}"
    ${OPENMPT_STATIC}
    ${MPG123_STATIC}
    ${SNDFILE_STATIC}
    ${FLAC_STATIC}
    ${VORBISFILE_STATIC}
    ${VORBIS_STATIC}
    ${OGG_STATIC}
    ${Z_STATIC}
    ${WINPTHREAD_STATIC}
    # Windows system libraries (always dynamic)
    opengl32 gdi32 winmm shlwapi user32
)
