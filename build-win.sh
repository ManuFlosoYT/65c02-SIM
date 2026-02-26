#!/bin/bash
set -e

BUILD_TYPE="Release"
CMAKE_OPTS=""
DO_CLEAN=false
for arg in "$@"; do
    if [[ "$arg" == "--clean" ]]; then
        DO_CLEAN=true
    elif [[ "$arg" == "--debug" ]]; then
        BUILD_TYPE="Debug"
    fi
done

if [[ "$BUILD_TYPE" == "Debug" ]]; then
    BUILD_DIR="build_win/debug"
else
    BUILD_DIR="build_win/release"
fi

if [ "$DO_CLEAN" = true ]; then
    echo "Cleaning build directory ($BUILD_DIR)..."
    rm -rf "$BUILD_DIR"
fi
if command -v ccache >/dev/null 2>&1; then
    echo "ccache found, enabling..."
    CMAKE_OPTS="-DCMAKE_CXX_COMPILER_LAUNCHER=ccache"
    ccache -z
fi

if command -v ninja >/dev/null 2>&1; then
    echo "Ninja found, using it as generator..."
    export CMAKE_GENERATOR=Ninja
    
    if [ -f "$BUILD_DIR/CMakeCache.txt" ]; then
        if grep -q "CMAKE_GENERATOR:INTERNAL=Unix Makefiles" "$BUILD_DIR/CMakeCache.txt"; then
            echo "Generator mismatch detected (Unix Makefiles -> Ninja). Cleaning build directory..."
            rm -rf "$BUILD_DIR"
        fi
    fi
fi

echo "Compiling for Windows (MinGW) in $BUILD_TYPE mode..."
cmake -S . -B "$BUILD_DIR" -DCMAKE_TOOLCHAIN_FILE=cmake/mingw-toolchain.cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE $CMAKE_OPTS
cmake --build "$BUILD_DIR" -j$(nproc)

if command -v ccache >/dev/null 2>&1; then
    echo "ccache statistics:"
    ccache -s
fi

echo "Running unit tests (Windows/Wine)..."
if command -v wine >/dev/null 2>&1; then
    wine "./$BUILD_DIR/unit_tests.exe"
else
    echo "Wine not found, skipping unit tests."
fi

mkdir -p output
cp "$BUILD_DIR/SIM_65C02.exe" output/ 2>/dev/null || true

echo "Cross-compilation for Windows ($BUILD_TYPE) completed in output"
exit 0