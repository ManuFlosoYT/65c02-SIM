# Building 65c02-SIM from Source

This guide provides a comprehensive walkthrough for building the **65c02-SIM** emulator on **Fedora Linux**, including cross-compilation for **Windows** and **WebAssembly**.

---

## Table of Contents

- [Overview & Architecture](#overview--architecture)
- [Prerequisites & Package Setup (Fedora)](#prerequisites--package-setup-fedora)
  - [Fedora (dnf)](#fedora-dnf)
  - [WebAssembly Prerequisites](#webassembly-prerequisites)
- [Multi-Worktree Repository Setup](#multi-worktree-repository-setup)
- [Unified Build Script (`build.sh`)](#unified-build-script-buildsh)
  - [Command-Line Options](#command-line-options)
  - [Linux Build](#linux-build)
  - [Windows Cross-Compilation](#windows-cross-compilation)
  - [WebAssembly Build](#webassembly-build)
  - [Headless Minimal NSF Runner](#headless-minimal-nsf-runner)
- [CMake Presets](#cmake-presets)
- [Toolchain & Compiler Optimizations](#toolchain--compiler-optimizations)
  - [C++23 Standard](#c23-standard)
  - [Mold Linker Integration](#mold-linker-integration)
  - [ccache Acceleration](#ccache-acceleration)
  - [Link-Time Optimization (LTO)](#link-time-optimization-lto)
- [Dependency Resolution (`Dependencies.cmake`)](#dependency-resolution-dependenciescmake)
- [Embedded Resources Pipeline](#embedded-resources-pipeline)
- [CI/CD Automation](#cicd-automation)

---

## Overview & Architecture

The 65c02-SIM build system is modularized using modern **CMake 3.24+** and **C++23**, accelerated with `mold` and `ccache`. The project is divided into clean CMake modules located in `cmake/`:

- `CompilerOptions.cmake`: Global C++23 standards, optimization flags, mold linker configuration, and ccache launchers.
- `Dependencies.cmake`: Automated external dependency acquisition via `FetchContent`.
- `EmbeddedResources.cmake`: Custom compilation targets embedding binary assets, CRT shaders, icons, and LLVM-MOS compiler headers into C++ sources.
- `CoreTarget.cmake`: Emulation engine static library (`65c02_core`).
- `FrontendTarget.cmake`: Graphical user interface executable (`SIM_65C02`) built with Dear ImGui, SDL3, and OpenGL.
- `UnitTests.cmake`: High-speed Unity Build testing harness using Google Test (`unit_tests`).
- `MinimalNSFRunner.cmake`: Lightweight headless target for NES Sound Format playback and subroutine tracing (`--minimal-core`).

---

## Prerequisites & Package Setup (Fedora)

### Fedora (dnf)

Fedora is the primary development environment. Install the necessary development tools, system libraries, cross-compilers, and asset generation utilities:

```bash
sudo dnf update -y
sudo dnf install -y \
    gcc-c++ libstdc++-static ninja-build cmake ccache mold \
    mesa-libGL-devel mesa-libGLU-devel mesa-libGLES-devel mesa-libEGL-devel \
    libX11-devel libXrandr-devel libXinerama-devel libXcursor-devel libXi-devel \
    libXext-devel libXfixes-devel libXScrnSaver-devel libXtst-devel \
    wayland-devel wayland-protocols-devel libxkbcommon-devel libdecor-devel \
    alsa-lib-devel pulseaudio-libs-devel pipewire-devel pipewire-jack-audio-connection-kit-devel \
    openssl-devel ffmpeg-devel zlib-ng-compat-static \
    ImageMagick librsvg2-tools dosfstools mtools jq wine \
    ucrt64-gcc ucrt64-gcc-c++ ucrt64-crt ucrt64-headers ucrt64-binutils \
    ucrt64-openssl ucrt64-openssl-static ucrt64-zlib ucrt64-zlib-static
```

> **Important (Fedora):** `libstdc++-static` is mandatory because Release builds statically link libstdc++ with `--whole-archive -lstdc++ --no-whole-archive` via `cmake/CompilerOptions.cmake`.

### WebAssembly Prerequisites

- **CMake 3.24+**
- **Python 3.9+**
- **Git**
- **Node.js** (Optional, provides `npx serve` for local testing)

The build script `./build.sh --target web` will automatically clone and activate a local isolated Emscripten SDK (`emsdk_local/`) if `emcmake` is not present in your system `PATH`.

---

## Multi-Worktree Repository Setup

65c02-SIM organizes auxiliary sub-projects (SDK, MicroDOS, MSBasic, development tools, assets, and wiki documentation) as **Git Worktrees** pointing to independent branches in the repository, rather than using git submodules:

```
65c02-SIM/                     [master branch] (Simulator C++ source & CMake build)
├── assets/                    [Assets branch]
├── sdk/                       [SDK branch]
│   ├── microdos/              [MicroDOS branch]
│   └── msbasic/               [MSBasic branch]
├── tools/                     [Tools branch]
└── wiki/                      [Wiki branch]
```

To set up a fresh clone with all 6 worktrees properly mounted:

```bash
# 1. Clone main simulator repository
git clone https://github.com/ManuFlosoYT/65c02-SIM.git
cd 65c02-SIM

# 2. Mount auxiliary worktrees
git worktree add assets Assets
git worktree add sdk SDK
git worktree add sdk/microdos MicroDOS
git worktree add sdk/msbasic MSBasic
git worktree add tools Tools
git worktree add wiki Wiki
```

### Why Worktrees?
- **Atomic Commits & Independence:** Each branch maintains its own revision history while remaining accessible in a single directory tree.
- **Zero Submodule Overhead:** No recursive checkout issues, detached `HEAD` traps, or out-of-sync submodule pointer hashes.
- **Root `.gitignore` Isolation:** The root `.gitignore` ignores `/sdk/`, `/tools/`, `/assets/`, and `/wiki/`, keeping the main repository clean.

---

## Unified Build Script (`build.sh`)

The repository includes a unified build script, `./build.sh`, which automates cache management, compiler launchers, target platform selection, tests, and artifact packaging.

### Command-Line Options

```bash
./build.sh [OPTIONS] [linux|win|web]
```

| Flag | Parameter / Format | Description |
|---|---|---|
| `--target` | `linux`, `win`, `web` | Specifies the compilation target platform (default: `linux`). |
| `--release` | None | Builds in optimized `Release` mode with LTO enabled (default). |
| `--debug` | None | Builds in `Debug` mode with full symbols and assertions. |
| `--clean` | None | Wipes the target build folder before compiling. |
| `--reconfigure` / `--configure` | None | Forces CMake to re-run configuration step. |
| `--minimal-core` | None | Builds only the lightweight headless NSF audio runner executable. |
| `--run` | None | For web builds, launches the local HTTP server and opens browser. |
| `-h`, `--help` | None | Displays help and usage information. |

### Build Directory Hierarchy

Build artifacts are separated by platform and configuration to prevent cache pollution:

| Target | Build Mode | Build Path | Output Executable |
|---|---|---|---|
| **Linux** | Release | `build/release/` | `output/SIM_65C02` |
| **Linux** | Debug | `build/debug/` | `output/SIM_65C02` |
| **Windows** | Release | `build_win/release/` | `output/SIM_65C02.exe` |
| **Windows** | Debug | `build_win/debug/` | `output/SIM_65C02.exe` |
| **WebAssembly** | Release / Debug | `build/web/` | `output/web/index.html` |
| **Minimal NSF** | Minimal Core | `build/nsf_core/` | `output/SIM_65C02` |

---

### Linux Build

To compile for Linux:

```bash
# Standard Release build
./build.sh

# Debug build with clean configuration
./build.sh --target linux --debug --clean

# Execute compiled emulator binary
./output/SIM_65C02
```

During Linux builds, the script:
1. Detects `ccache` and configures it with a 15 GB cache ceiling (`CCACHE_SLOPPINESS="pch_defines,time_macros"`).
2. Detects `Ninja` as the high-speed build generator.
3. Enforces the `mold` linker for sub-second linking.
4. Compiles `65c02_core`, `imgui_lib`, and `SIM_65C02`.
5. Executes the complete Google Test suite via `ctest --test-dir <build_dir> -j $(nproc) --output-on-failure`.
6. Copies the finished binary to `output/SIM_65C02`.

---

### Windows Cross-Compilation

To cross-compile a 64-bit Windows binary from Linux using MinGW-w64 UCRT:

```bash
# Standard Release Windows build
./build.sh --target win

# Clean Debug Windows build
./build.sh --target win --debug --clean
```

The Windows cross-compilation pipeline:
- Applies `cmake/mingw-toolchain.cmake`, utilizing `x86_64-w64-mingw32ucrt-gcc` and `x86_64-w64-mingw32ucrt-g++`.
- Statically links GCC runtime libraries (`-static -static-libgcc -static-libstdc++`) and sets an 8 MB thread stack (`-Wl,--stack,8388608`).
- Links against required Windows API libraries: `ws2_32`, `crypt32`, `pathcch`, `bcrypt`, and `opengl32`.
- Automatically fetches prebuilt FFmpeg Windows shared builds (BtbN); CMake copies the runtime DLLs (`avcodec-*.dll`, `avformat-*.dll`, `avutil-*.dll`, `swscale-*.dll`) into the build directory (`build_win/release/`). When deploying `output/SIM_65C02.exe`, keep these DLLs in the same folder.
- Runs unit tests under `wine` if installed on the host system.

---

### WebAssembly Build

To compile the simulator into WebAssembly:

```bash
# Compile WebAssembly build
./build.sh --target web

# Compile, launch local server, and open in browser
./build.sh --target web --run
```

The WebAssembly compilation process:
- Checks for an activated Emscripten environment. If missing, automatically downloads, installs, and activates `emsdk_local/`.
- Configures Emscripten with POSIX threads (`-pthread`), Web Workers (`-sPTHREAD_POOL_SIZE=8`), WebGL 2.0 (`-sMAX_WEBGL_VERSION=2`), and dynamic memory allocation up to 4 GB (`-sMAXIMUM_MEMORY=4294967296`).
- Embeds HTML5 shell template `src/Frontend/web/shell.html`.
- Copies Cross-Origin Isolation service worker `coi-serviceworker.js` to `output/web/`.
- Catalogs existing cartridges, MIDI files, and VRAM files into `output/web/roms/roms.json` using `jq`.
- When `--run` is supplied, starts `npx serve` (or `python3 -m http.server`) on port `8080` and opens `http://localhost:8080`.

For complete details on browser features and Cross-Origin Isolation, refer to [WebAssembly Build](WebAssembly-Build).

---

### Headless Minimal NSF Runner

To build a standalone headless runner with minimal dependencies for NES Sound Format execution and subroutine tracing:

```bash
./build.sh --minimal-core
```

This bypasses GUI and audio window libraries (`SDL3`, `ImGui`, `OpenGL`), compiling only:
- `NSFRunner_main.cpp`
- `NSFRunner.cpp`
- `CPU.cpp`
- Hardware bus and profiler components (`Bus.cpp`, `SubroutineProfiler.cpp`, `PageCrossTracker.cpp`, `SymbolTable.cpp`, `DbgFormatParser.cpp`).

Output binary is placed in `output/SIM_65C02`.

---

## CMake Presets

The repository defines native presets in `CMakePresets.json` (schema version 3, CMake 3.24+). You can invoke CMake directly without using `build.sh`:

```bash
# Configure using presets
cmake --preset linux-release
cmake --preset linux-debug
cmake --preset windows-release
cmake --preset windows-debug

# Build using presets
cmake --build --preset linux-release -j$(nproc)
cmake --build --preset linux-debug -j$(nproc)
cmake --build --preset windows-release -j$(nproc)
cmake --build --preset windows-debug -j$(nproc)
```

---

## Toolchain & Compiler Optimizations

### C++23 Standard

65c02-SIM is written in **modern C++23**. Configured in `cmake/CompilerOptions.cmake`:

```cmake
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)
```

Modern C++ features utilized across the codebase include `std::span`, `std::expected`, multidimensional subscripts, nested namespaces, and enhanced constexpr algorithms.

### Mold Linker Integration

On Linux, `cmake/CompilerOptions.cmake` mandates the use of the **mold** high-speed linker:

```cmake
if(UNIX AND NOT EMSCRIPTEN)
    find_program(MOLD_PATH mold REQUIRED)
    set(CMAKE_LINKER_TYPE MOLD)
    add_link_options("-fuse-ld=mold")
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        add_link_options("-Wl,--whole-archive" "-lstdc++" "-Wl,--no-whole-archive")
    endif()
endif()
```

Additionally, `build.sh` automatically detects `mold` on Linux and passes `-DCMAKE_LINKER_TYPE=MOLD`.

Mold provides a 4x to 10x link-time acceleration over `gold` and `lld`, enabling virtually instantaneous incremental rebuilds when editing frontend or core code.

### ccache Acceleration

`ccache` is integrated at both the CMake level and inside `build.sh`:

```cmake
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
endif()
```

`build.sh` manages ccache cache invalidation and statistics:
- Sets `CCACHE_SLOPPINESS="pch_defines,time_macros"` to ensure compatibility with precompiled headers (`target_precompile_headers`).
- Caps maximum cache size at 15 GB (`ccache -M 15G`).
- Displays cache hit ratios after build completion via `ccache -s`.
- Clears cache counters on clean builds via `ccache -z`.

### Link-Time Optimization (LTO)

In `Release` configurations, Interprocedural Optimization (IPO/LTO) is enabled across all targets:
- Native Linux / Windows: `-flto=auto -Wl,-O3 -Wl,--as-needed -Wl,--gc-sections -s`
- Function and loop alignment: `-falign-functions=32 -falign-loops=32 -falign-jumps=32 -falign-labels=32`
- Symbol visibility: `-fvisibility=hidden -fvisibility-inlines-hidden`
- Dead code stripping: `-ffunction-sections -fdata-sections`

---

## Dependency Resolution (`Dependencies.cmake`)

External libraries are fetched and built automatically during the first CMake run via `FetchContent`. Network updates are locked (`FETCHCONTENT_UPDATES_DISCONNECTED ON`) to ensure deterministic rebuilds:

| Dependency | Version / Branch | Purpose | Integration Method |
|---|---|---|---|
| **SDL3** | `release-3.4.x` | Audio callbacks, window creation, keyboard/mouse events | FetchContent + `patch_sdl3.cmake` |
| **Dear ImGui** | `docking` branch | Dockable GUI panels, memory editor, debugger windows | FetchContent static library (`imgui_lib`) |
| **ImGuiFileDialog** | `v0.6.8` | Cross-platform file picker modal dialogs | FetchContent + `patch_imguifiledialog.cmake` |
| **ImGuiColorTextEdit**| `master` | In-app assembly and script code editor | FetchContent + `patch_imguicolortextedit.cmake` |
| **nlohmann/json** | `v3.12.0` | Cartridge manifests, profiler export, configuration JSON | FetchContent (header-only archive) |
| **Glad** | `v2.0.8` | OpenGL 3.3 Core and GLES loader generator | FetchContent + `GladExtensions.cmake` |
| **Miniz** | `3.1.1` | ZIP cartridge decompression and save-state compression | FetchContent + `patch_miniz.cmake` |
| **ASIO** | `asio-1-38-0` | Standalone asynchronous I/O for ESP8266 Wi-Fi emulation | FetchContent (`ASIO_STANDALONE`) |
| **cpp-httplib** | `v0.41.0` | HTTP/HTTPS client for online update checks and web assets | FetchContent + OpenSSL support |
| **Google Test** | `main` | Unit testing framework (`GTest::gtest_main`) | FetchContent |
| **LLVM-MOS SDK** | `v23.3.0` | Embedded C compiler toolchain (`mos-clang`) for 6502 | FetchContent / System compiler detection |
| **FFmpeg** | `master-latest` | Video/audio recording and GIF export | System pkg-config (Linux) / BtbN Zip (Windows) |
| **OpenSSL** | System / 3.x | TLS encryption for HTTP client | System package |

---

## Embedded Resources Pipeline

The build system utilizes a custom Python utility (`cmake/embed_resource.py`), invoked via `cmake/EmbeddedResources.cmake`, to package static assets directly into the C++ binary:

1. **CRT Shaders (`CRTShaders.h`):** Vertex and fragment GLSL shaders (`crt.vert`, `crt.frag`) are converted into string constants.
2. **Icons (`IconPixels.h` & `resource.rc`):** Application SVG vector artwork (`65c02-sim.svg`) is rasterized at build time to 256x256 PNG and multi-resolution ICO via `rsvg-convert` or ImageMagick, generating raw pixel arrays for SDL3 window icons and native Windows resource files.
3. **LLVM-MOS Compiler VFS (`embedded_llvm_mos.h`):** On desktop builds, standard library headers, runtime libraries (`sdk/src/Libs`, `sdk/microdos/Libs`), linker scripts (`sdk/linker/link.ld`), and BIOS definitions (`sdk/linker/bios.s`) are embedded into a Virtual File System header, allowing the built-in IDE to compile 6502 C code out-of-the-box without requiring an external toolchain.
4. **Cascadia Mono Font (`CascadiaMono.cpp`):** Base85-encoded monospace font data compiled into source memory.
5. **SID Wavetables (`SIDWaveTables.cpp`):** High-precision precalculated wavetables for the MOS 6581/8580 SID sound generator.
6. **SDK Bundle (`sdk_zip.h`):** Packages `SDK.zip` into the binary so users can extract SDK files directly from the UI.

---

## CI/CD Automation

Continuous Integration and Release Packaging are managed via **GitHub Actions** (`.github/workflows/release.yml`):
- **Triggers:** Execution runs on release version tags (`v*`) or via manual trigger (`workflow_dispatch`).
- **Pipeline Workflow:**
  1. **Minimal Core (`build-core-nsf`):** Compiles the headless core executable for NSF sound execution.
  2. **SDK Packaging (`sdk`):** Generates toolchain libraries, includes MicroDOS, MSBasic, and packages `SDK.zip`.
  3. **Linux Build (`build-linux`):** Compiles using mold and runs unit tests via `ctest`.
  4. **Windows Cross-Build & Test (`build-windows`, `test-windows`):** Compiles MinGW UCRT64 executable and validates the test suite under Wine.
  5. **WebAssembly Deployment (`build-web`):** Compiles the Wasm SPA and deploys the build folder to the `gh-pages` branch.
  6. **GitHub Release (`release`):** Attaches `SIM_65C02` (Linux), `SIM_65C02.exe` (Windows), and `SDK.zip` to the GitHub release.
  7. **Wiki Synchronization (`update-wiki`):** Automatically syncs documentation from the `Wiki` git branch to the live GitHub Wiki repository.
