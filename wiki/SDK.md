# Developer SDK & Toolchain

The 65c02-SIM SDK provides a modern development environment targeting the Western Design Center W65C02S CPU. The toolchain is powered by **LLVM-MOS** (`mos-clang`), bringing modern C23 support, aggressive Link-Time Optimization (LTO), dead-code elimination, and ELF debugging symbols to 8-bit development.

---

## 1. Toolchain Architecture

The SDK replaces legacy 6502 compilers (such as cc65) with the LLVM-MOS compiler infrastructure.

```
+--------------------------------------------------------------------------+
|                              Source Code                                 |
|      C Programs (.c)             Assembly (.s)            Assets         |
|   (Modern C23 / C++23)         (GNU Assembler)       (PNG, MIDI, NSF)    |
+--------------------------------------------------------------------------+
                                    |
                                    v
+--------------------------------------------------------------------------+
|                           SDK Build Scripts                              |
|   compile-bin.sh   image-to-bin.sh   midi-to-bin.sh   create-cartridge.sh |
+--------------------------------------------------------------------------+
                                    |
                                    v
+--------------------------------------------------------------------------+
|                          LLVM-MOS Toolchain                              |
|   mos-clang / mos-common-clang  |  llvm-objcopy  |  llvm-objdump         |
|   -mcpu=mosw65c02               |  -flto -Os/-Oz |  --gc-sections        |
+--------------------------------------------------------------------------+
                                    |
                                    v
+--------------------------------------------------------------------------+
|                             Output Artifacts                             |
|    output/rom/*.bin (Flat binary)         output/rom/*.elf (Symbols/GDB) |
|    output/apps/*.app (microDOS App)       output/cartridge/*.65c (ROM)   |
+--------------------------------------------------------------------------+
```

### Compiler Discovery & Environment Variables
The build scripts automatically locate the LLVM-MOS toolchain binaries by searching the following locations in order:
1. `$MOS_CLANG` environment variable (if exported).
2. Local build directory: `build/llvm_mos/llvm-mos/bin/mos-common-clang`.
3. Toolchain bundle: `output/toolchain/bin/mos-common-clang`.
4. System PATH: `mos-common-clang` or `mos-clang`.

To explicitly point the SDK to a custom LLVM-MOS installation:
```bash
export MOS_CLANG="/opt/llvm-mos/bin/mos-clang"
export LLVM_MOS_DIR="/opt/llvm-mos"
```

### SDK & Toolchain Directory Structure

```
65c02-SIM/
├── sdk/
│   ├── linker/                 # Runtime startup files and linker base scripts
│   │   ├── link.ld             # Base GNU linker script for LLVM-MOS
│   │   ├── App-Runtime.s       # MicroDOS application entry, header, and trampolines
│   │   ├── C-Runtime.s         # Minimal C runtime initialization
│   │   ├── bios.s              # Standard Ben Eater ACIA/VIA BIOS assembly
│   │   └── microdos_bios.s     # MicroDOS kernel BIOS and ABI jump table stubs
│   ├── microdos/               # MicroDOS operating system sources
│   │   ├── Apps/               # 47 built-in microDOS applications (.c and .png icons)
│   │   ├── Libs/               # OS headers (app-bios.h, tui.h, Font5x5.h, Font3x5.h)
│   │   ├── System/             # Desktop GUI environment (desktop_sys.c, desktop_vectors.s)
│   │   └── src/                # Kernel entry point and shell (microDOS.c, shell.c, run_cmd.c)
│   ├── msbasic/                # Microsoft BASIC 6502 interpreter sources
│   ├── scripts/                # Automated developer build scripts
│   │   ├── compile-bin.sh      # Universal LLVM-MOS compiler script
│   │   ├── create-cartridge.sh # .65c cartridge packaging tool
│   │   ├── create-microdos-cartridge.sh # Complete microDOS OS cartridge builder
│   │   ├── create-sdk.sh       # Release distribution packager
│   │   ├── image-to-bin.sh     # PNG/JPG to VRAM 100x75 RGB222 converter
│   │   └── midi-to-bin.sh      # MIDI/NSF to SID music compiler
│   └── src/                    # Standalone SDK demo programs and hardware tests
│       ├── Libs/               # Hardware abstraction headers (GPU, SID, VIA, LCD, NET)
│       └── *.c, *.s            # Example sources (cube.c, room.c, game.c, tetris.c...)
├── tools/                      # Offline developer tooling
│   ├── linker/                 # Linker script generators (generate_link_ld.py, generate_app_link_ld.py)
│   ├── microDOS/               # Asset tools (image_to_icon.py, build_udos_sys.py)
│   └── sid/                    # Audio compression tools (audio_to_sid.py, nsf_processor.py)
└── output/                     # Build artifacts directory
    ├── apps/                   # Compiled microDOS applications (*.app)
    ├── cartridge/              # Packaged cartridges (*.65c)
    ├── img/                    # FAT16 disk images (microDOS.img)
    ├── rom/                    # Flat ROM binaries (*.bin) and ELF symbols (*.elf)
    ├── system/                 # System components (DESKTOP.SYS)
    └── vram/                   # Converted graphics binaries (*.bin)
```

---

## 2. Compiling Programs (`sdk/scripts/compile-bin.sh`)

[`sdk/scripts/compile-bin.sh`](file:///home/manu/Documents/GitHub/65c02-SIM/sdk/scripts/compile-bin.sh) is the central compilation script. It supports building standalone ROM binaries, microDOS applications, assembly sources, and system firmware.

### Usage
```bash
./sdk/scripts/compile-bin.sh <program_name> [options]
```

### Common Targets & Flags
- `./sdk/scripts/compile-bin.sh <name>`: Compiles `sdk/src/<name>.c` or `sdk/src/<name>.s` to `output/rom/<name>.bin` and `output/rom/<name>.elf`.
- `./sdk/scripts/compile-bin.sh <name> --microDOS`: Compiles `sdk/microdos/Apps/<name>.c` into a relocatable microDOS application (`output/apps/<name>.app`).
- `./sdk/scripts/compile-bin.sh microDOS`: Compiles the microDOS kernel ROM (`output/rom/microDOS.bin`).
- `./sdk/scripts/compile-bin.sh desktop_sys`: Compiles the graphical desktop environment (`output/system/DESKTOP.SYS`).
- `./sdk/scripts/compile-bin.sh eater`: Compiles the classic WozMon + Microsoft BASIC ROM (`output/rom/eater.bin`).
- `./sdk/scripts/compile-bin.sh all`: Batch compiles all SDK programs and firmwares.
- `-multithread` / `--multithread`: Enables multi-threaded parallel compilation when batching.
- `-unity` / `--unity`: Merges C sources into a single translation unit for maximum whole-program LTO.

---

## 3. Compiler Optimization & Overflow Fallback

Because 6502 programs run within constrained 32 KB or 64 KB memory maps, `compile-bin.sh` implements an intelligent **optimization fallback cascade**:

```
+-----------------------------------------------+
| Attempt 1: -g -Os (Size opt + Debug symbols)  |
+-----------------------------------------------+
                       | (Memory overflow?)
                       v
+-----------------------------------------------+
| Attempt 2: -g -Oz (Aggressive min-size + Dbg) |
+-----------------------------------------------+
                       | (Memory overflow?)
                       v
+-----------------------------------------------+
| Attempt 3: -Os (Strip debug symbols)          |
+-----------------------------------------------+
                       | (Memory overflow?)
                       v
+-----------------------------------------------+
| Attempt 4: -Oz (Stripped aggressive min-size) |
+-----------------------------------------------+
                       | (Still overflow?)
                       v
              [Compilation Error]
```

### Key LLVM-MOS Compiler Flags Used
- `-mcpu=mosw65c02`: Enables 65C02-specific instructions (e.g. `STZ`, `BRA`, `PHX`, `PHY`, `PLX`, `PLY`).
- `-flto` & `-Wl,--lto-O2`: Link-Time Optimization performs cross-module inlining and dead-function stripping.
- `-ffunction-sections` & `-fdata-sections`: Emits every function and variable into its own linker section.
- `-Wl,--gc-sections`: Eliminates unused functions and data from the final binary.
- `-fno-unroll-loops`: Disables loop unrolling to preserve code density.
- `-fnonreentrant`: Optimizes local stack frames by allocating non-reentrant local variables to static zero-page/BSS where safe.

---

## 4. Hardware Detection & Dynamic Linker Generation

When compiling standard C programs (`sdk/src/<name>.c`), the SDK scans the source file for hardware library inclusions and dynamically generates a collision-free linker script using [`tools/linker/generate_link_ld.py`](file:///home/manu/Documents/GitHub/65c02-SIM/tools/linker/generate_link_ld.py):

| Detected Header | Linker Flag | Reserved Memory Block | Purpose |
|---|---|---|---|
| `#include "Libs/GPU.h"` | `--gpu` | `$2000–$3FFF` (8 KB) | Single-buffered VRAM front buffer |
| `#include "Libs/GPUDoubleBuffer.h"`| `--double-buffer` | `$2000–$3FFF` & `$6010–$790F` | Front buffer + Back buffer |
| `#include "Libs/NET.h"` | `--net` | `$5004–$5007` (4 B) | ESP8266 WiFi UART registers |
| `#include "Libs/SID.h"` | `--sid` | `$4800–$481F` (32 B) | MOS 6581/8580 sound registers |
| `#include "Libs/SD.h"` | (Automatic) | Links FatFs stack | `ff.c`, `diskio.c`, and `SD.c` |

The script calculates free RAM ranges between reserved peripherals and creates contiguous linker pools (`ram_1`, `ram_2`, etc.) automatically.

---

## 5. Cartridge Packaging (`sdk/scripts/create-cartridge.sh`)

The emulator uses `.65c` cartridge files (compressed ZIP archives containing a `manifest.json` and associated asset binaries). [`sdk/scripts/create-cartridge.sh`](file:///home/manu/Documents/GitHub/65c02-SIM/sdk/scripts/create-cartridge.sh) automates this process.

### Syntax
```bash
./sdk/scripts/create-cartridge.sh <ROM_FILE> [options]
```

### Options
- `--name "<Title>"`: Cartridge display name.
- `--author "<Author>"`: Creator name.
- `--desc "<Description>"`: Description text.
- `--version "<Version>"`: Version string (e.g. `1.0`).
- `--type <rom|vram|midi|nsf>`: Cartridge classification.
- `--ips <Frequency>`: CPU clock frequency in Hz (default: `1000000` = 1 MHz; e.g. `10000000` = 10 MHz).
- `--gpu <true|false>`: Enable hardware graphics controller.
- `--sid <true|false>`: Enable SID sound chip.
- `--sid-model <6581|8580>`: Select SID hardware revision.
- `--esp <true|false>`: Enable ESP8266 WiFi emulation.
- `--sd <true|false>`: Enable SD card SPI interface.
- `--sd-image <path.img>`: Attach a FAT16 SD card disk image.
- `--microdos`: Configures microDOS bus memory mappings ($7FC0–$7FFF MMIO).

### Cartridge File Structure (`.65c`)
```
game.65c (ZIP container)
├── manifest.json      # Hardware parameters and memory bus map
├── rom.bin            # 32 KB ROM binary ($8000-$FFFF)
├── rom.elf            # ELF debug symbols for the internal debugger
├── vram.bin           # (Optional) Preloaded graphics image
└── sdcard.img         # (Optional) Attached 32MB FAT16 storage image
```

---

## 6. Asset Conversion Tools

### Converting Images to VRAM (`sdk/scripts/image-to-bin.sh`)
Converts modern image formats (PNG, JPG, BMP) into the simulator's raw 100x75 RGB222 format (1 byte per pixel, 7500 bytes total).

```bash
# Convert a single image:
./sdk/scripts/image-to-bin.sh assets/vram/wallpaper.png

# Convert all images in assets/vram/:
./sdk/scripts/image-to-bin.sh all
```
Output is stored in `output/vram/<name>.bin`.

### Converting Audio to SID (`sdk/scripts/midi-to-bin.sh`)
Converts standard MIDI files (`.mid`) or NES Sound Format (`.nsf`) files into native 65C02 SID music players or raw `.sid` byte streams for microDOS.

```bash
# Compile a MIDI file to a standalone playable ROM cartridge:
./sdk/scripts/midi-to-bin.sh assets/midi/song.mid

# Compile all MIDIs for microDOS streaming playback (/sid/song.sid):
./sdk/scripts/midi-to-bin.sh all --microDOS
```

The converter supports 5 optimization modes (`max_quality`, `quality`, `balanced`, `compressed`, `max_compression`) and automatically selects the highest fidelity mode that fits into the 32 KB ROM window.

---

## 7. Automated microDOS Cartridge Builder (`sdk/scripts/create-microdos-cartridge.sh`)

[`sdk/scripts/create-microdos-cartridge.sh`](file:///home/manu/Documents/GitHub/65c02-SIM/sdk/scripts/create-microdos-cartridge.sh) builds a complete, production-ready microDOS operating system cartridge from scratch:

1. Compiles the microDOS kernel ROM with LLVM-MOS.
2. Compiles all 47 applications in `sdk/microdos/Apps/` to `.app` binaries with custom 16x16 icons.
3. Compiles `DESKTOP.SYS`.
4. Creates a 32 MB FAT16 SD card disk image (`output/img/microDOS.img`) using `mkfs.fat`.
5. Pre-populates the SD card with directories (`/bin`, `/sid`, `/vram`, `/SYSTEM`, `/SYSTEM/DESKTOP`, `/SYSTEM/SWAP`).
6. Copies system wallpapers, desktop binaries, audio tracks, and apps into the SD image using `mtools`.
7. Packages the kernel and disk image into `output/cartridge/microDOS.65c`.

---

## 8. Complete SDK Distribution Builder (`sdk/scripts/create-sdk.sh`)

To package the entire SDK and all compiled binaries into a distributable `SDK.zip` release archive:

```bash
./sdk/scripts/create-sdk.sh
```

This script:
- Compiles all C and assembly programs in `sdk/src/`.
- Converts all images and MIDI audio tracks.
- Assembles standalone `.65c` cartridges for all demos.
- Generates the full `microDOS.65c` cartridge.
- Archives the tools, sources, headers, and pre-compiled assets into `SDK.zip`.
