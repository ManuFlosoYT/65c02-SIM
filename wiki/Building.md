# Building — Compilar desde el código fuente

## Prerrequisitos

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

> **Windows:** Usa WSL2 con Fedora para ejecutar los scripts de compilación. Consulta el README para las instrucciones de instalación.

## Clonar el repositorio

```bash
git clone https://github.com/ManuFlosoYT/65c02-SIM
cd 65c02-SIM
```

## Compilar para Linux

```bash
./build-linux.sh
```

El ejecutable resultante se guarda en `output/linux/SIM_65C02`.

### Opciones del script

| Opción | Descripción |
|--------|-------------|
| `--clean` | Fuerza una compilación limpia (elimina la caché de CMake) |
| `--debug` | Compila con símbolos de depuración (sin optimizaciones) |

```bash
./build-linux.sh --clean     # Compilación limpia
./build-linux.sh --debug     # Compilación de depuración
```

El script:
1. Detecta **ccache** automáticamente para compilaciones incrementales más rápidas
2. Detecta **Ninja** para compilación paralela
3. Ejecuta los **tests unitarios** al final de la compilación
4. Reporta errores si algún test falla

## Compilar para Windows (MinGW cross-compilation)

```bash
./build-win.sh
```

El ejecutable resultante se guarda en `output/windows/SIM_65C02.exe`.

```bash
./build-win.sh --clean       # Compilación limpia
```

Si **Wine** está instalado, el script ejecuta automáticamente los tests unitarios.

## Sistema de compilación (CMake)

El proyecto usa **CMake** con los siguientes objetivos:

| Target | Tipo | Descripción |
|--------|------|-------------|
| `65c02_core` | Biblioteca estática | Motor de emulación (Hardware + CPU) |
| `imgui_lib` | Biblioteca estática | Dear ImGui + backends SDL3/OpenGL |
| `SIM_65C02` | Ejecutable | Simulador completo con GUI |
| `unit_tests` | Ejecutable | Suite de tests unitarios (Google Test) |

### Estándar y optimizaciones

- **C++20** (`set_property(TARGET ... CXX_STANDARD 20)`)
- **LTO** (Link-Time Optimization) en Release
- Flags de Release: `-O3 -DNDEBUG -ffunction-sections -fdata-sections -flto -Wl,--gc-sections -s`

### Dependencias externas (FetchContent)

CMake descarga automáticamente todas las dependencias durante la primera compilación:

| Dependencia | Versión | Uso |
|-------------|---------|-----|
| SDL3 | 3.4.x | Ventana, eventos, audio |
| ImGui | docking branch | Interfaz gráfica |
| ImGuiFileDialog | 0.6.4 | Diálogo de archivos |
| GLAD | 2.0.8 | Cargador de OpenGL |
| nlohmann/json | 3.12.0 | Parseo JSON |
| cpp-httplib | 0.15.3 | HTTP/HTTPS |
| GoogleTest | latest | Tests unitarios |

> La primera compilación puede tardar varios minutos debido a la descarga y compilación de dependencias.

## Estructura de salida

```
output/
├── linux/
│   └── SIM_65C02          ← Ejecutable Linux
├── windows/
│   └── SIM_65C02.exe      ← Ejecutable Windows
└── rom/                   ← Programas compilados con compile-bin.sh
    ├── echo.bin
    ├── tetris.bin
    └── ...
```

## CI/CD

El proyecto usa **GitHub Actions** (`.github/workflows/`) para:
- Compilar automáticamente en cada push
- Ejecutar los tests unitarios
- Generar los artefactos de release (Linux + Windows + SDK)
