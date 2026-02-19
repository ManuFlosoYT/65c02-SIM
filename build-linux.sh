#!/bin/bash
set -e

# Detect ccache
CMAKE_OPTS=""

# Handle arguments
for arg in "$@"; do
    if [[ "$arg" == "--clean" ]]; then
        echo "Cleaning build directory..."
        rm -rf build
    elif [[ "$arg" == "--debug" ]]; then
        echo "Enabling debug symbols..."
        CMAKE_OPTS="$CMAKE_OPTS -DCMAKE_BUILD_TYPE=Debug"
    fi
done

if command -v ccache >/dev/null 2>&1; then
    echo "ccache found, enabling..."
    CMAKE_OPTS="$CMAKE_OPTS -DCMAKE_CXX_COMPILER_LAUNCHER=ccache"
    ccache -z # Zero stats
fi

# Detect Ninja
if command -v ninja >/dev/null 2>&1; then
    echo "Ninja found, using it as generator..."
    export CMAKE_GENERATOR=Ninja
    
    # Check for generator mismatch in existing cache
    if [ -f build/CMakeCache.txt ]; then
        if grep -q "CMAKE_GENERATOR:INTERNAL=Unix Makefiles" build/CMakeCache.txt; then
            echo "Generator mismatch detected (Unix Makefiles -> Ninja). Cleaning build directory..."
            rm -rf build
        fi
    fi
fi

# Linux Build
echo "Compiling for Linux..."
cmake -S . -B build $CMAKE_OPTS
cmake --build build -j$(nproc)

if command -v ccache >/dev/null 2>&1; then
    echo "ccache statistics:"
    ccache -s
fi

echo "Running unit tests (Linux)..."
./build/unit_tests

# Copy Output
mkdir -p output/linux
cp build/SIM_65C02 output/linux/ 2>/dev/null || true

echo "Linux build completed in output/linux"
exit 0
