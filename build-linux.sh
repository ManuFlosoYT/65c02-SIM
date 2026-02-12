#!/bin/bash
set -e

# Detect ccache
CMAKE_OPTS=""
if command -v ccache >/dev/null 2>&1; then
    echo "ccache found, enabling..."
    CMAKE_OPTS="-DCMAKE_CXX_COMPILER_LAUNCHER=ccache"
fi

# Linux Build
echo "Compiling for Linux..."
cmake -S . -B build $CMAKE_OPTS
cmake --build build -j$(nproc)
echo "Running unit tests (Linux)..."
./build/unit_tests

# Copy Output
mkdir -p output/linux
cp build/SIM_65C02 output/linux/ 2>/dev/null || true

echo "Linux build completed in output/linux"
exit 0
