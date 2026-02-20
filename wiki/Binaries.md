# Binaries — Example Programs

**Directory:** `Binaries/`

## Overview

The `Binaries/` directory contains example programs written in **C** and **65c02 assembly** that demonstrate the emulator's capabilities. They are the ideal starting point for developing new programs.

All programs are compiled with `./compile-bin.sh <name>` and the resulting binary is saved to `output/rom/<name>.bin`.

---

## C programs

### `echo.c` — Basic serial echo
Minimal reference program. Reads characters from the serial input (ACIA) and echoes them back to the console. Useful as a starting template for new projects.

### `fullTest.c` — Full integration test
Integration test that exercises multiple hardware components at once: ACIA, VIA, timers, etc. Useful to verify that the emulator is working correctly.

### `testVIA.c` — VIA 6522 test
Demonstrates the use of the VIA chip's timers and I/O ports. Includes examples of Timer 1 configuration (continuous mode) and Timer 2 (one-shot).

### `testGPU.c` — GPU test
Demonstrates how to write pixels to VRAM and display images or graphics in the **VRAM Viewer** window. Requires the `Libs/GPU.h` library.

### `testSID.c` — SID test
Demonstrates the use of the sound chip: oscillator configuration, waveforms, and ADSR envelopes.

### `fillVRAM.c` — VRAM fill
Fills the entire VRAM with a pixel pattern. Useful for testing graphics write performance and the correct synchronisation with the GPU blanking interval.

### `mat.c` — Matrix multiplication (8-bit)
Implements matrix multiplication using 8-bit integers. Demonstrates CPU performance on mathematical operations and the use of arrays in memory.

### `mat64.c` — Matrix multiplication (64-bit)
Same as `mat.c` but with 64-bit integers emulated in software. Useful for benchmarking the 65c02 on extended-precision arithmetic.

### `cube.c` — Rotating 3D cube
Renders a rotating 3D cube in VRAM using fixed-point maths. Requires the `Libs/GPU.h` library. An excellent real-time graphics demonstration.

### `room.c` — 3D scene
Renders a simple 3D scene in VRAM using ray casting or a similar technique. Requires `Libs/GPU.h`.

### `game.c` — Game
Interactive game that uses serial input and the GPU for rendering. Demonstrates the classic game loop: input → update → render.

### `tetris.c` — Tetris
Full implementation of the **Tetris** game for the emulator. Uses VRAM for graphics and the ACIA for keyboard input. One of the most complete demonstrations of the simulator.

---

## Assembly programs

### `vga.s` — Assembly VGA driver
Low-level GPU driver written directly in 65c02 assembly. Shows how to access VRAM directly and control synchronisation timings.

---

## Special target: `eater`

```bash
./compile-bin.sh eater
```

Compiles the historic **WozMon + Microsoft BASIC** ROM as used by Ben Eater in his 65c02 project. The resulting binary (`output/rom/eater.bin`) boots directly into the interactive BASIC interpreter.

---

## Support libraries (`Binaries/Libs/`)

| File | Description |
|------|-------------|
| `BIOS.h` | Declarations for BIOS routines (CHROUT, CHRIN, etc.) |
| `GPU.h` | VRAM access with a single buffer |
| `GPUDoubleBuffer.h` | VRAM access with double buffering (no tearing) |
| `LCD.h` | Functions for writing to the LCD display |
| `SID.h` | SID sound chip control |
| `VIA.h` | Access to VIA 6522 registers |

---

## Creating a new program

### Minimal C program

```c
// Binaries/hello.c
#include "Libs/BIOS.h"

int main() {
    // CHROUT writes a character to the serial console
    CHROUT('H'); CHROUT('e'); CHROUT('l');
    CHROUT('l'); CHROUT('o'); CHROUT('!');
    CHROUT('\n');
    return 0;
}
```

```bash
./compile-bin.sh hello
# Output: output/rom/hello.bin
```

### Program with GPU

```c
// Binaries/pixels.c
#include "Libs/GPU.h"

int main() {
    int x, y;
    for (y = 0; y < 75; y++)
        for (x = 0; x < 100; x++)
            vram[y][x] = (x + y) & 0xFF;  // diagonal gradient
    return 0;
}
```

```bash
./compile-bin.sh pixels
```
