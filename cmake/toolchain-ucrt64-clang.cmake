set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_C_COMPILER   "C:/msys64/ucrt64/bin/clang.exe")
set(CMAKE_CXX_COMPILER "C:/msys64/ucrt64/bin/clang++.exe")
set(CMAKE_RC_COMPILER  "C:/msys64/ucrt64/bin/windres.exe")

# Ensure CMake finds UCRT64 packages, not system ones
set(CMAKE_SYSROOT      "C:/msys64/ucrt64")
set(CMAKE_FIND_ROOT_PATH "C:/msys64/ucrt64")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
