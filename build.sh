#!/bin/bash
set -e

TARGET_OS="linux"
BUILD_TYPE="Release"
CMAKE_OPTS=""
DO_CLEAN=false
DO_RUN=false
FORCE_CONFIGURE=false

for arg in "$@"; do
    if [[ "$arg" == "--clean" ]]; then
        DO_CLEAN=true
    elif [[ "$arg" == "--debug" ]]; then
        BUILD_TYPE="Debug"
    elif [[ "$arg" == "--run" ]]; then
        DO_RUN=true
    elif [[ "$arg" == "--reconfigure" || "$arg" == "--configure" ]]; then
        FORCE_CONFIGURE=true
    elif [[ "$arg" == "--target" ]]; then
        # Skip this iteration, handle below
        continue
    elif [[ "$arg" == "linux" || "$arg" == "win" || "$arg" == "web" ]]; then
        # Check if previous arg was --target. A simple way:
        TARGET_OS="$arg"
    elif [[ "$arg" == "--target="* ]]; then
        TARGET_OS="${arg#*=}"
    fi
done

# Secondary pass for simple --target <val>
for ((i=1; i<=$#; i++)); do
    if [[ "${!i}" == "--target" ]]; then
        next_i=$((i+1))
        if [ $next_i -le $# ]; then
            TARGET_OS="${!next_i}"
        fi
    fi
done

echo "Building for OS: $TARGET_OS in $BUILD_TYPE mode"

# Setup Build Directory based on OS and TYPE
if [[ "$TARGET_OS" == "win" ]]; then
    if [[ "$BUILD_TYPE" == "Debug" ]]; then BUILD_DIR="build_win/debug"; else BUILD_DIR="build_win/release"; fi
elif [[ "$TARGET_OS" == "web" ]]; then
    BUILD_DIR="build/web"
else # linux default
    if [[ "$BUILD_TYPE" == "Debug" ]]; then BUILD_DIR="build/debug"; else BUILD_DIR="build/release"; fi
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

if [[ "$TARGET_OS" == "web" ]]; then
    EMSDK_DIR="emsdk_local"
    if ! command -v emcmake >/dev/null 2>&1; then
        if [ ! -d "$EMSDK_DIR" ]; then
            echo "Emscripten SDK not found. Downloading to $EMSDK_DIR..."
            git clone --depth 1 https://github.com/emscripten-core/emsdk.git "$EMSDK_DIR"
            cd "$EMSDK_DIR"
            ./emsdk install latest
            ./emsdk activate latest
            cd ..
        fi
        echo "Activating Emscripten SDK..."
        source "$EMSDK_DIR/emsdk_env.sh" > /dev/null 2>&1
    fi

    if ! command -v emcmake >/dev/null 2>&1; then
        echo "Error: Failed to activate Emscripten. Please try manual installation."
        exit 1
    fi

    mkdir -p "$BUILD_DIR"
    if [ ! -f "$BUILD_DIR/CMakeCache.txt" ] || [ "$FORCE_CONFIGURE" = true ]; then
        emcmake cmake -S . -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=$BUILD_TYPE $CMAKE_OPTS
    else
        echo "Skipping CMake configure (cache found). Use --reconfigure to force it."
    fi
    cmake --build "$BUILD_DIR" -j$(nproc)

    mkdir -p output/web
    cp "$BUILD_DIR/SIM_65C02.html" output/web/index.html 2>/dev/null || true
    cp "$BUILD_DIR/SIM_65C02.js" output/web/ 2>/dev/null || true
    cp "$BUILD_DIR/SIM_65C02.wasm" output/web/ 2>/dev/null || true
    cp "$BUILD_DIR/SIM_65C02.worker.js" output/web/ 2>/dev/null || true
    cp "src/Frontend/web/coi-serviceworker.js" output/web/ 2>/dev/null || true
    cp "src/Frontend/Assets/65c02-sim.svg" output/web/favicon.svg 2>/dev/null || true

    echo "Web build completed in output/web (index.html)"

    mkdir -p output/web/roms/midi
    mkdir -p output/web/roms/vram
    [ -d "output/cartridge" ] && cp output/cartridge/*.65c output/web/roms/ 2>/dev/null || true
    [ -d "output/rom" ] && cp output/rom/*.bin output/web/roms/ 2>/dev/null || true
    [ -d "output/midi" ] && cp output/midi/*.65c output/web/roms/midi/ 2>/dev/null || true
    [ -d "output/vram" ] && cp output/vram/*.65c output/web/roms/vram/ 2>/dev/null || true

    if command -v jq >/dev/null 2>&1; then
        echo "Generating structured roms.json..."
        ROMS_JSON=$(ls output/cartridge/*.65c 2>/dev/null | xargs -n 1 basename | jq -R . | jq -s . || echo "[]")
        MIDIS_JSON=$(ls output/midi/*.65c 2>/dev/null | xargs -n 1 basename | jq -R . | jq -s . || echo "[]")
        VRAMS_JSON=$(ls output/vram/*.65c 2>/dev/null | xargs -n 1 basename | jq -R . | jq -s . || echo "[]")
        jq -n --argjson r "$ROMS_JSON" --argjson m "$MIDIS_JSON" --argjson v "$VRAMS_JSON" \
            '{roms: $r, midis: $m, vrams: $v}' > output/web/roms/roms.json
    fi

    if [ "$DO_RUN" = true ]; then
        PORT=8080
        echo "Starting local server at http://localhost:$PORT..."
        if command -v xdg-open > /dev/null; then xdg-open "http://localhost:$PORT" &
        elif command -v open > /dev/null; then open "http://localhost:$PORT" & fi
        if command -v npx > /dev/null; then npx -y serve output/web -l $PORT
        else python3 -m http.server $PORT --directory output/web; fi
    fi

elif [[ "$TARGET_OS" == "win" ]]; then
    if [ ! -f "$BUILD_DIR/CMakeCache.txt" ] || [ "$FORCE_CONFIGURE" = true ]; then
        cmake -S . -B "$BUILD_DIR" -DCMAKE_TOOLCHAIN_FILE=cmake/mingw-toolchain.cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE $CMAKE_OPTS
    else
        echo "Skipping CMake configure (cache found). Use --reconfigure to force it."
    fi
    cmake --build "$BUILD_DIR" -j$(nproc)

    if command -v ccache >/dev/null 2>&1; then ccache -s; fi

    echo "Running unit tests (Windows/Wine)..."
    if command -v wine >/dev/null 2>&1; then
        wine "./$BUILD_DIR/unit_tests.exe"
    else
        echo "Wine not found, skipping unit tests."
    fi

    mkdir -p output
    cp "$BUILD_DIR/SIM_65C02.exe" output/ 2>/dev/null || true
    echo "Windows build completed in output"

else
    # Linux build
    if [ ! -f "$BUILD_DIR/CMakeCache.txt" ] || [ "$FORCE_CONFIGURE" = true ]; then
        cmake -S . -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=$BUILD_TYPE $CMAKE_OPTS
    else
        echo "Skipping CMake configure (cache found). Use --reconfigure to force it."
    fi
    cmake --build "$BUILD_DIR" -j$(nproc)

    if command -v ccache >/dev/null 2>&1; then ccache -s; fi

    echo "Running unit tests (Linux)..."
    "./$BUILD_DIR/unit_tests"

    mkdir -p output
    cp "$BUILD_DIR/SIM_65C02" output/ 2>/dev/null || true
    echo "Linux build completed in output"
fi

exit 0
