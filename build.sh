#!/bin/bash
set -e

# 1. Linux Build
echo "Compiling for Linux..."
cmake -S . -B build
cmake --build build -j$(nproc)
echo "Running unit tests (Linux)..."
./build/unit_tests

# 2. Windows Build (Cross-compile)
echo "Compiling for Windows (MinGW)..."
cmake -S . -B build_win -DCMAKE_TOOLCHAIN_FILE=mingw-toolchain.cmake -DCMAKE_BUILD_TYPE=Release
cmake --build build_win -j$(nproc)

# 3. Copy Output
mkdir -p output/linux
mkdir -p output/windows

cp build/SIM_65C02 output/linux/ 2>/dev/null || true
cp build_win/SIM_65C02.exe output/windows/ 2>/dev/null || true

echo "Builds completed in output/"
exit 0