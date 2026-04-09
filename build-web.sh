#!/bin/bash
set -e

BUILD_TYPE="Release"
CMAKE_OPTS=""
DO_CLEAN=false
DO_RUN=false

for arg in "$@"; do
    if [[ "$arg" == "--clean" ]]; then
        DO_CLEAN=true
    elif [[ "$arg" == "--debug" ]]; then
        BUILD_TYPE="Debug"
    elif [[ "$arg" == "--run" ]]; then
        DO_RUN=true
    fi
done

BUILD_DIR="build/web"

if [ "$DO_CLEAN" = true ]; then
    echo "Cleaning build directory ($BUILD_DIR)..."
    rm -rf "$BUILD_DIR"
fi

# --- Emscripten Automatic Setup ---
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



echo "Compiling for WebAssembly in $BUILD_TYPE mode..."
mkdir -p "$BUILD_DIR"

emcmake cmake -S . -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=$BUILD_TYPE $CMAKE_OPTS
cmake --build "$BUILD_DIR" -j$(nproc)

mkdir -p output/web
cp "$BUILD_DIR/SIM_65C02.html" output/web/index.html 2>/dev/null || true
cp "$BUILD_DIR/SIM_65C02.js" output/web/ 2>/dev/null || true
cp "$BUILD_DIR/SIM_65C02.wasm" output/web/ 2>/dev/null || true
cp "$BUILD_DIR/SIM_65C02.worker.js" output/web/ 2>/dev/null || true
cp "Frontend/web/coi-serviceworker.js" output/web/ 2>/dev/null || true
cp "Frontend/Assets/65c02-sim.svg" output/web/favicon.svg 2>/dev/null || true

echo "Web build completed in output/web (index.html)"

# Copy Cartridges, MIDIs, and VRAMs if they exist
mkdir -p output/web/roms/midi
mkdir -p output/web/roms/vram
[ -d "output/cartridge" ] && cp output/cartridge/*.65c output/web/roms/ 2>/dev/null || true
[ -d "output/rom" ] && cp output/rom/*.bin output/web/roms/ 2>/dev/null || true
[ -d "output/midi" ] && cp output/midi/*.65c output/web/roms/midi/ 2>/dev/null || true
[ -d "output/vram" ] && cp output/vram/*.65c output/web/roms/vram/ 2>/dev/null || true

if command -v jq >/dev/null 2>&1; then
    echo "Generating structured roms.json..."
    # Prioritize .65c cartridges for the ROMs list
    ROMS_JSON=$(ls output/cartridge/*.65c 2>/dev/null | xargs -n 1 basename | jq -R . | jq -s . || echo "[]")
    
    # Keep others for compatibility or direct loading if needed
    MIDIS_JSON=$(ls output/midi/*.65c 2>/dev/null | xargs -n 1 basename | jq -R . | jq -s . || echo "[]")
    VRAMS_JSON=$(ls output/vram/*.65c 2>/dev/null | xargs -n 1 basename | jq -R . | jq -s . || echo "[]")
    
    jq -n --argjson r "$ROMS_JSON" --argjson m "$MIDIS_JSON" --argjson v "$VRAMS_JSON" \
        '{roms: $r, midis: $m, vrams: $v}' > output/web/roms/roms.json
else
    echo "Warning: jq not found, skipping structured roms.json generation."
fi

if [ "$DO_RUN" = true ]; then
    PORT=8080
    echo "Starting local server at http://localhost:$PORT..."
    
    # Open browser in background
    if command -v xdg-open > /dev/null; then
        xdg-open "http://localhost:$PORT" &
    elif command -v open > /dev/null; then
        open "http://localhost:$PORT" &
    fi

    # Start server
    if command -v npx > /dev/null; then
        echo "Using 'npx serve'..."
        npx -y serve output/web -l $PORT
    else
        echo "Using 'python3 -m http.server'..."
        python3 -m http.server $PORT --directory output/web
    fi
fi

exit 0
