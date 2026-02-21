#!/bin/bash
set -e

# Default BUILD_TYPE
BUILD_TYPE="Release"
CMAKE_OPTS=""

# Handle arguments
for arg in "$@"; do
    if [[ "$arg" == "--clean" ]]; then
        echo "Cleaning build directory..."
        rm -rf build_win
    elif [[ "$arg" == "--debug" ]]; then
        echo "Enabling debug symbols..."
        BUILD_TYPE="Debug"
    fi
done

# Detect ccache
if command -v ccache >/dev/null 2>&1; then
    echo "ccache found, enabling..."
    CMAKE_OPTS="-DCMAKE_CXX_COMPILER_LAUNCHER=ccache"
    ccache -z # Zero stats
fi

# Detect Ninja (optional for Windows cross-compile, but usually preferred if available)
if command -v ninja >/dev/null 2>&1; then
    echo "Ninja found, using it as generator..."
    export CMAKE_GENERATOR=Ninja
    # Check for generator mismatch in existing cache
    if [ -f build_win/CMakeCache.txt ]; then
        if grep -q "CMAKE_GENERATOR:INTERNAL=Unix Makefiles" build_win/CMakeCache.txt; then
            echo "Generator mismatch detected (Unix Makefiles -> Ninja). Cleaning build directory..."
            rm -rf build_win
        fi
    fi
fi

# Windows Build (Cross-compile)
echo "Compiling for Windows (MinGW)..."
cmake -S . -B build_win -DCMAKE_TOOLCHAIN_FILE=mingw-toolchain.cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE $CMAKE_OPTS
cmake --build build_win -j$(nproc)

if command -v ccache >/dev/null 2>&1; then
    echo "ccache statistics:"
    ccache -s
fi

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
