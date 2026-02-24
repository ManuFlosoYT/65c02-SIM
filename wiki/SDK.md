# SDK — Development Tools

The SDK includes three automation scripts for compiling programs and converting assets for the emulator.

> **Windows:** All scripts are Bash scripts. Windows users must use **WSL2** (see the installation section in the [README](https://github.com/ManuFlosoYT/65c02-SIM/blob/master/README.md)).

---

## `compile-bin.sh` — Compile programs

Compiles **C** or **65c02 assembly** programs and generates a `.bin` binary ready to load into the simulator.

### Usage

```bash
./compile-bin.sh <program_name>
./compile-bin.sh all          # Compile all programs
./compile-bin.sh eater        # Compile the WOZMON + Microsoft BASIC ROM
```

Output is saved to `output/rom/<name>.bin`.

### Requirements

- **cc65** (`ca65`, `ld65`, `cl65`) — 6502 assembly/compilation toolchain
- **Python 3** (for the conversion scripts)

### Compilation flow

#### Assembly programs (`.s`)

```
Binaries/<name>.s
       ↓ ca65 --cpu 65C02
Binaries/build/<name>.o
       ↓ ld65 -C Linker/raw.cfg
output/rom/<name>.bin
```

#### C programs (`.c`)

```
Binaries/<name>.c
       ↓ cl65 -O --cpu 65C02 -S     (compile to assembly)
Binaries/build/<name>.s
       ↓ cl65 --cpu 65C02 -C <cfg>  (assemble + link + BIOS)
       │  Includes: Linker/bios.s + Linker/C-Runtime.s
output/rom/<name>.bin
```

#### Automatic linker detection

The script detects which linker configuration to use based on the headers included in the program:

| Detected header                     | Configuration used              |
| ----------------------------------- | ------------------------------- |
| `#include "Libs/GPUDoubleBuffer.h"` | `C-Runtime-GPUDoubleBuffer.cfg` |
| `#include "Libs/GPU.h"`             | `C-Runtime-GPU.cfg`             |
| (none)                              | `C-Runtime.cfg`                 |

#### Special target `eater`

Compiles the **WozMon + Microsoft BASIC** ROM using the Makefile in `Linker/msbasic/`.

---

## `image-to-bin.sh` — Convert images to VRAM

Converts an image (PNG, JPG, or BMP) to the emulator's VRAM binary format (100×75 pixels, 1 byte per pixel in RGB 222).

### Usage

```bash
./image-to-bin.sh <image_name>
./image-to-bin.sh all         # Convert all images in GPU/
# Example: ./image-to-bin.sh GPU/bocchi.png
```

Output is saved to `output/vram/<image_name>.bin`.

### Requirements

- **Python 3** with **Pillow** (`pip install Pillow`)

### Conversion process

1. Load the image with Pillow
2. Resize to 100×75 pixels
3. Convert to RGB 222 (2 bits per color channel)
4. Save the byte array as a binary file

The resulting binary can be loaded directly into VRAM through the graphical interface.

---

## `midi-to-bin.sh` — Convert MIDI to SID code

Converts a MIDI file to 65c02 assembly code for the emulator's SID chip. The result can be compiled and run directly.

### Usage

```bash
./midi-to-bin.sh <midi_file>
./midi-to-bin.sh all          # Convert all MIDI files in SID/
# Example: ./midi-to-bin.sh SID/overworld.mid
```

Output is saved to `output/midi/<song>.bin`.

### Requirements

- **Python 3** with **mido** (`pip install mido`)
- **cc65** (`ca65`, `ld65`) to compile the generated assembly

### Optimization modes

The script has **8 compression modes** (time granularities from 1 ms to 100+ ms). It automatically tries from the most precise mode to the most compressed and selects the first one that fits in 32 KB of ROM:

| Mode | Granularity | Fidelity            |
| ---- | ----------- | ------------------- |
| 1    | ~1 ms       | Maximum fidelity    |
| 2    | ~2 ms       | High fidelity       |
| 3    | ~5 ms       | Good fidelity       |
| 4    | ~10 ms      | Medium fidelity     |
| 5    | ~20 ms      | Medium compression  |
| 6    | ~40 ms      | High compression    |
| 7    | ~80 ms      | Very compressed     |
| 8    | ~100+ ms    | Maximum compression |

If no mode fits within 32 KB, the script reports an error.

### Conversion process

1. Parse the MIDI file with `mido`
2. Generate 65c02 assembly code that writes to SID registers to play the song
3. Compile with `ca65` + `ld65` using `Linker/raw.cfg`
4. The final binary is loaded into ROM and the CPU executes the instructions to produce sound

---

## C libraries for programs

`Binaries/Libs/` contains C header files that make it easy to use the hardware from C code:

| File                | Description                                   |
| ------------------- | --------------------------------------------- |
| `GPU.h`             | Functions for writing to VRAM (single buffer) |
| `GPUDoubleBuffer.h` | Functions for double-buffered VRAM access     |
| `SID.h`             | Functions for controlling the SID chip        |
| `VIA.h`             | Functions for accessing the VIA 6522          |

---

## Pre-compiled Assets

The **SDK.zip** distributed in the official releases already contains the `output/` folder with all programs, images, and music pre-compiled. You can find them in:

- `output/rom/`: Main program binaries.
- `output/vram/`: Converted images for the GPU.
- `output/midi/`: Compiled music for the SID chip.
