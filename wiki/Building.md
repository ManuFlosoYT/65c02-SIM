# Building — Compile from Source

## Prerequisites

### Fedora (dnf)

```bash
sudo dnf update
sudo dnf install gcc-c++ ninja-build mesa-libGL-devel mesa-libGLU-devel \
    libX11-devel libXrandr-devel libXinerama-devel libXcursor-devel libXi-devel \
    wayland-devel libxkbcommon-devel \
    mingw64-gcc mingw32-gcc \
    alsa-lib-devel pulseaudio-libs-devel \
    cmake cc65 openssl-devel \
    mingw64-openssl mingw64-openssl-static mingw64-zlib-static \
    wget curl git tar gcc git-core make \
    fribidi-devel pipewire-devel libXext-devel libXfixes-devel \
    libXScrnSaver-devel libXtst-devel dbus-devel ibus-devel systemd-devel \
    mesa-libGLES-devel mesa-libEGL-devel vulkan-devel \
    wayland-protocols-devel libdrm-devel mesa-libgbm-devel \
    libusb1-devel libdecor-devel \
    pipewire-jack-audio-connection-kit-devel libthai-devel liburing-devel \
    zlib-ng-compat-static python3-jinja2
```

> **Windows:** Use WSL2 with Fedora to run the build scripts. See the README for installation instructions.

## Clone the repository

```bash
git clone https://github.com/ManuFlosoYT/65c02-SIM
cd 65c02-SIM
```

## Build for Linux

```bash
./build-linux.sh
```

The resulting executable is saved to `output/linux/SIM_65C02`.

### Script options

| Option | Description |
|--------|-------------|
| `--clean` | Forces a clean build (removes the CMake cache) |
| `--debug` | Builds with debug symbols |
| `--clean --debug` | Forces a clean build and builds with debug symbols |

```bash
./build-linux.sh                     # Build (default)
./build-linux.sh --clean             # Clean build
./build-linux.sh --debug             # Debug build
./build-linux.sh --clean --debug     # Clean build and debug build
```

The script:
1. Automatically detects **ccache** for faster incremental builds
2. Detects **Ninja** for parallel compilation
3. Runs the **unit tests** at the end of the build
4. Reports errors if any test fails

## Build for Windows (MinGW cross-compilation)

```bash
./build-win.sh
```

The resulting executable is saved to `output/windows/SIM_65C02.exe`.

```bash
./build-win.sh                     # Build (default)
./build-win.sh --clean             # Clean build
./build-win.sh --debug             # Debug build
./build-win.sh --clean --debug     # Clean build and debug build
```

If **Wine** is installed, the script automatically runs the unit tests.

## Build system (CMake)

The project uses **CMake** with the following targets:

| Target | Type | Description |
|--------|------|-------------|
| `65c02_core` | Static library | Emulation engine (Hardware + CPU) |
| `imgui_lib` | Static library | Dear ImGui + SDL3/OpenGL backends |
| `SIM_65C02` | Executable | Full simulator with GUI |
| `unit_tests` | Executable | Unit test runner (Google Test) |

### Standard and optimizations

- **C++20** (`set_property(TARGET ... CXX_STANDARD 20)`)
- **LTO** (Link-Time Optimization) in Release mode
- Release flags: `-O3 -DNDEBUG -ffunction-sections -fdata-sections -flto -Wl,--gc-sections -s`

### External dependencies (FetchContent)

CMake automatically downloads all dependencies during the first build:

| Dependency | Version | Use |
|------------|---------|-----|
| SDL3 | 3.4.x | Window, events, audio |
| ImGui | docking branch | GUI framework |
| ImGuiFileDialog | 0.6.4 | File dialog |
| GLAD | 2.0.8 | OpenGL loader |
| nlohmann/json | 3.12.0 | JSON parsing |
| cpp-httplib | 0.15.3 | HTTP/HTTPS |
| GoogleTest | latest | Unit testing |

> The first build may take several minutes due to downloading and compiling dependencies.

## Output structure

```
output/
├── linux/
│   └── SIM_65C02          ← Linux executable
├── windows/
│   └── SIM_65C02.exe      ← Windows executable
└── rom/                   ← Programs compiled with compile-bin.sh
    ├── echo.bin
    ├── tetris.bin
    └── ...
```

## CI/CD

The project uses **GitHub Actions** (`.github/workflows/`) to:
- Automatically build on every push
- Run the unit tests
- Generate release artifacts (Linux + Windows + SDK)
