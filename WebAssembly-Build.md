# WebAssembly Build (Emscripten)

The 65C02-SIM can be compiled to WebAssembly using the Emscripten toolchain, allowing it to run directly in modern web browsers with high performance and multi-threading support.

## Prerequisites

The build script automatically detects or downloads the **Emscripten SDK (emsdk)** into a local folder (`emsdk_local/`). You do not need to install it manually, but you do need:

- **CMake** (3.24+)
- **Python 3**
- **Git**
- **Node.js** (Optional, for `npx serve` during local testing)

## Building the Web Port

Run the provided script:

```bash
./scripts/build-web.sh
```

This will:
1. Setup the Emscripten environment.
2. Configure CMake for the `TARGET_WASM` platform.
3. Compile the simulator into `output/web/`.

### Output Files
- `index.html`: The main entry point.
- `SIM_65C02.js`: The Emscripten glue code.
- `SIM_65C02.wasm`: The compiled binary.
- `SIM_65C02.worker.js`: Worker script for pthreads.
- `coi-serviceworker.js`: Enables Cross-Origin Isolation (required for multi-threading).

## Running Locally

Due to security restrictions (Cross-Origin Isolation for `SharedArrayBuffer`), you cannot simply open the `index.html` file from your disk. You must serve it through a web server with specific headers.

You can use the built-in run command:

```bash
./scripts/build-web.sh --run
```

This script will start a local server (prioritizing `npx serve` or falling back to `python3 -m http.server`) and automatically open your browser at the correct URL.

## Browser Features & Limitations

### Native File Access
The web port uses a custom JavaScript bridge (`WebFileUtils.h`) to bypass the server's filesystem. 
- **Loading ROMs/Scripts**: Opens a native browser file picker.
- **Saving States/SD Images**: Triggers a browser download of the file.
- **SD Card Persistence**: Since browsers cannot write directly back to your local files, modifications are kept in a virtual filesystem. Use **"Unmount & Save"** or **"Save Changes"** in the Control Window to download the updated image.

### Multi-threading
The simulator uses **pthreads** for the emulation core, SID audio, and UI updates. This requires a browser that supports `SharedArrayBuffer` and **Cross-Origin Isolation**. The included `coi-serviceworker.js` handles this automatically by re-loading the page with the necessary headers.

### Limitations
- **FFmpeg/Recording**: Video and audio recording to disk is currently disabled in the web port due to the heavy dependencies of FFmpeg. Buttons are visible but disabled with an explanatory tooltip.
- **ESP8266 (WiFi)**: The ESP8266 chip is disabled for the web port as it requires host-side socket access that the browser cannot provide. Its row in the **Memory Layout** table is rendered non-interactive with an explanatory tooltip.
- **Networking**: High-level networking features (OpenSSL/httplib) are disabled to reduce binary size and avoid library incompatibilities.

## GitHub Pages Deployment

The project is configured to deploy automatically via GitHub Actions. Every time a new version tag is pushed, the `.github/workflows/release.yml` workflow builds the Wasm version and pushes it to the `gh-pages` branch.
