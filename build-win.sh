#!/bin/bash
set -e

# Detect ccache
CMAKE_OPTS=""
if command -v ccache >/dev/null 2>&1; then
    echo "ccache found, enabling..."
    CMAKE_OPTS="-DCMAKE_CXX_COMPILER_LAUNCHER=ccache"
fi

# Windows Build (Cross-compile)
echo "Compiling for Windows (MinGW)..."
cmake -S . -B build_win -DCMAKE_TOOLCHAIN_FILE=mingw-toolchain.cmake -DCMAKE_BUILD_TYPE=Release $CMAKE_OPTS
cmake --build build_win -j$(nproc)
echo "Running unit tests (Windows/Wine)..."
# Check if wine is installed
if command -v wine >/dev/null 2>&1; then
    wine ./build_win/unit_tests.exe
else
    echo "Wine not found, skipping unit tests."
fi

# Copy Output
mkdir -p output/windows
cp build_win/SIM_65C02.exe output/windows/ 2>/dev/null || true

echo "Windows build completed in output/windows"
exit 0
