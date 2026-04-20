#!/bin/bash
set -e

BUILD_TYPE="Release"
CMAKE_OPTS=""
DO_CLEAN=false
FORCE_CONFIGURE=false
PERFDEBUG_LTO="OFF"
PERF_TUNE_NATIVE="OFF"
PGO_GENERATE="OFF"
PGO_USE="OFF"

for arg in "$@"; do
    if [[ "$arg" == "--clean" ]]; then
        DO_CLEAN=true
    elif [[ "$arg" == "--debug" ]]; then
        BUILD_TYPE="Debug"
    elif [[ "$arg" == "--perfdebug" ]]; then
        BUILD_TYPE="PerfDebug"
        PERFDEBUG_LTO="OFF"
    elif [[ "$arg" == "--perfdebug-lto" ]]; then
        BUILD_TYPE="PerfDebug"
        PERFDEBUG_LTO="ON"
    elif [[ "$arg" == "--native" ]]; then
        PERF_TUNE_NATIVE="ON"
    elif [[ "$arg" == "--pgo-generate" ]]; then
        PGO_GENERATE="ON"
        PGO_USE="OFF"
    elif [[ "$arg" == "--pgo-use" ]]; then
        PGO_GENERATE="OFF"
        PGO_USE="ON"
    elif [[ "$arg" == "--reconfigure" || "$arg" == "--configure" ]]; then
        FORCE_CONFIGURE=true
    fi
done

if [[ "$BUILD_TYPE" == "Debug" ]]; then
    BUILD_DIR="build/debug"
elif [[ "$BUILD_TYPE" == "PerfDebug" ]]; then
    if [[ "$PERFDEBUG_LTO" == "ON" ]]; then
        BUILD_DIR="build/perfdebug-lto"
    else
        BUILD_DIR="build/perfdebug"
    fi
else
    if [[ "$PGO_GENERATE" == "ON" ]]; then
        BUILD_DIR="build/release-pgo-generate"
    elif [[ "$PGO_USE" == "ON" ]]; then
        BUILD_DIR="build/release-pgo-use"
    elif [[ "$PERF_TUNE_NATIVE" == "ON" ]]; then
        BUILD_DIR="build/release-native"
    else
        BUILD_DIR="build/release"
    fi
fi

if [ "$DO_CLEAN" = true ]; then
    echo "Cleaning build directory ($BUILD_DIR)..."
    rm -rf "$BUILD_DIR"
fi

if command -v ccache >/dev/null 2>&1; then
    echo "ccache found, enabling..."
    CMAKE_OPTS="$CMAKE_OPTS -DCMAKE_CXX_COMPILER_LAUNCHER=ccache"
    ccache -M 15G
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

echo "Compiling for Linux in $BUILD_TYPE mode..."
if [ ! -f "$BUILD_DIR/CMakeCache.txt" ] || [ "$FORCE_CONFIGURE" = true ]; then
    cmake -S . -B "$BUILD_DIR" \
        -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
        -DPERFDEBUG_ENABLE_LTO=$PERFDEBUG_LTO \
        -DPERF_TUNE_NATIVE=$PERF_TUNE_NATIVE \
        -DPGO_GENERATE=$PGO_GENERATE \
        -DPGO_USE=$PGO_USE \
        $CMAKE_OPTS
else
    echo "Skipping CMake configure (cache found). Use --reconfigure to force it."
fi
cmake --build "$BUILD_DIR" -j$(nproc)

if command -v ccache >/dev/null 2>&1; then
    echo "ccache statistics:"
    ccache -s
fi

echo "Running unit tests (Linux)..."
"./$BUILD_DIR/unit_tests"

mkdir -p output
cp "$BUILD_DIR/SIM_65C02" output/ 2>/dev/null || true

echo "Linux build completed in output"
exit 0
