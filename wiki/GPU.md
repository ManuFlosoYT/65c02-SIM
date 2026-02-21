# GPU — Graphics Processor and VRAM

**File:** `Hardware/GPU.h` / `Hardware/GPU.cpp`  
**Namespace:** `Hardware::GPU`

## Overview

The emulator includes a **custom graphics processor** with 7.5 KB of VRAM. It can display color images at a resolution of 100×75 pixels. The GUI renders the VRAM contents in real time in the **VRAM Viewer** window.

## Dimensions

| Parameter | Value | Description |
|-----------|-------|-------------|
| `VRAM_WIDTH` | 100 px | Visible width |
| `VRAM_HEIGHT` | 75 px | Visible height |
| `DISPLAY_WIDTH` | 132 px | Total width including blanking |
| `DISPLAY_HEIGHT` | 78 px | Total height including blanking |
| `VRAM_HEIGHT_DRAWABLE_BY_CPU` | 64 rows | Rows accessible by the CPU via the bus |

The **blanking** area (132×78) simulates the retrace interval of an analog monitor. During the blanking interval the bus is free and the CPU can write to VRAM without conflicts.

## VRAM layout in the address space

VRAM occupies the range `0x2000`–`0x3FFF` of the system address space (configured in the GPU linker files).

Pixel addressing within VRAM is:
```
bits [6:0]   → X coordinate (0–99)
bits [13:7]  → Y coordinate (0–74)
```

For example, to access pixel (x=5, y=10):
```
addr = (10 << 7) | 5 = 0x0505
effective address = 0x2000 + 0x0505 = 0x2505
```

## Pixel format

Each VRAM byte represents **1 pixel** (RGB 222).

## Writing from the CPU

The CPU can write directly to VRAM while the GPU is in the blanking interval. `IsInBlankingInterval()` indicates when it is safe to do so:

```asm
; Wait for blanking interval (65c02 assembly example)
WAIT_BLANK:
    LDA $2000,Y
    ; The blanking status bit indicates when it is safe to write
    BEQ WAIT_BLANK
    STA $2000,Y     ; Write pixel
```

In practice, the simulator assumes the CPU can always access the first 64 rows without conflict.

## Image conversion tool

The `GPU/generator/` directory contains a Python script that converts PNG, JPG, or BMP images to the VRAM binary format.

```bash
./image-to-bin.sh <image_name>
# Output: output/vram/<image_name>.bin
```

The resulting binary can be loaded directly into VRAM through the graphical interface.

## Sample images

Several demonstration images are included in `GPU/`

## Double Buffering

The linker config `C-Runtime-GPUDoubleBuffer.cfg` enables **double-buffer** mode: while the GPU reads from the first buffer, the CPU writes to the second, eliminating tearing.

## Integration with the Frontend

The **VRAM Viewer** window in the GUI reads the `gpu.vram[y][x]` array directly and renders it as an OpenGL texture, updated every frame.
