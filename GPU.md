# GPU — Graphics Processor and VRAM Architecture

**Source Files:**
- Header: [`src/Hardware/Video/GPU.h`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Hardware/Video/GPU.h)
- Implementation: [`src/Hardware/Video/GPU.cpp`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Hardware/Video/GPU.cpp)
- Raster Timing: [`src/Hardware/Video/RasterTimingGenerator.h`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Hardware/Video/RasterTimingGenerator.h)
- microDOS Subclass: [`src/Hardware/Video/MicroDosGPU.h`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Hardware/Video/MicroDosGPU.h) / [`src/Hardware/Video/MicroDosGPU.cpp`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Hardware/Video/MicroDosGPU.cpp)
- Asset Tools: [`tools/gpu/image-to-bin.py`](file:///home/manu/Documents/GitHub/65c02-SIM/tools/gpu/image-to-bin.py), [`tools/gpu/64-color.act`](file:///home/manu/Documents/GitHub/65c02-SIM/tools/gpu/64-color.act)

---

## 1. Overview

The 65c02-SIM platform features a custom memory-mapped Graphics Processing Unit (GPU) equipped with **7.5 KB of Video RAM (VRAM)**. It drives an indexed color display with an active visible resolution of **100 × 75 pixels** at 1 byte per pixel (RGB 2:2:2, 64 colors).

The GPU subsystem models the timing characteristics of vintage cathode-ray tube (CRT) controllers, including horizontal and vertical blanking intervals, electron beam tracking, CPU wait states, and asynchronous dirty-frame tracking for high-performance host OpenGL rendering.

Additionally, a specialized subclass—**`MicroDosGPU`**—provides zero-wait-state bus operations, 8 KB of supplementary auxiliary RAM, and direct integration with an emulated hardware PS/2 mouse controller.

---

## 2. Hardware Dimensions and Geometry

The GPU raster engine divides screen geometry into the visible display area and blanking retrace intervals:

| Geometric Parameter | Value | Definition / Hardware Role |
|:---|:---:|:---|
| `VRAM_WIDTH` | **100 px** | Visible pixel width per horizontal scanline |
| `VRAM_HEIGHT` | **75 px** | Visible pixel height (total raster lines displayed) |
| `DISPLAY_WIDTH` | **132 cycles** | Total scanline period: 100 active cycles + 32 H-Blank cycles |
| `DISPLAY_HEIGHT` | **78 lines** | Total frame period: 64 CPU-drawable lines + 14 V-Blank lines |
| `VRAM_HEIGHT_DRAWABLE_BY_CPU` | **64 rows** | Rows directly mappable into the standard 8 KB CPU aperture ($64 \times 128 = 8192\text{ bytes}$) |
| CPU-Visible Active Window | **6,400 bytes (6.25 KB)** | $100 \times 64$ pixels directly accessible within the 8 KB CPU window |
| Total Display VRAM | **7,500 bytes (~7.5 KB)** | Full $100 \times 75$ pixels active display surface (1 byte per pixel) |
| Linear VRAM Storage | **9,600 bytes** | Total memory footprint with 128-byte row stride ($75 \times 128$) |
| Physical VRAM Stride | **128 bytes** | Row stride aligned to power-of-two address decoding ($2^7$) |

```
0            99   100                     131
+-------------+---------------------------+  ---
|             |                           |   ^
|   VISIBLE   |     Horizontal Blank      |   |  64 CPU-drawable rows
|    PIXEL    |        (H-BLANK)          |   |  (Active drawing)
|    DATA     |         32 cycles         |   v
+-------------+---------------------------+  ---  ---
| Rows 64-74  |                           |   ^
| (Bottom 11) |                           |   |  11 bottom rows
+-------------+                           |   |  + 3 V-Blank lines
|                                         |   |  (Total 14 lines V-BLANK)
|      Vertical Blanking Interval         |   |
|               (V-BLANK)                 |   v
+-----------------------------------------+  ---
```

---

## 3. Memory Mapping & Address Decoding

### 3.1 Bus Placement

In the standard system configuration, the GPU is mapped as an `IBusDevice` across an 8 KB address window:

- **Default Address Window:** `$2000` – `$3FFF` (8,192 bytes)
- **microDOS Cartridge System:** Mapped at `$5FC0` – `$7FBF` (`APP_EXTRA_RAM_START` to `APP_EXTRA_RAM_END`, 8,192 bytes) as `microDOS-GPU`. Standalone custom cartridges can map `GPU` or `microDOS-GPU` at any 8 KB boundary.

### 3.2 Hardware Coordinate Decoding Formula

The GPU uses hardware bitfield splitting rather than integer division or multiplication. When the CPU issues a read or write to address `addr` (relative to the GPU base address):

$$\begin{aligned}
X &= \text{addr} \ \& \ \text{0x7F} \quad &&(\text{bits } [6:0], \text{ range } 0 \dots 127) \\
Y &= (\text{addr} \gg 7) \ \& \ \text{0x7F} \quad &&(\text{bits } [13:7], \text{ range } 0 \dots 127)
\end{aligned}$$

```
Address Bit Layout:
15 14 13 12 11 10  9  8  7  6  5  4  3  2  1  0
[ Base ] [    Y (0..127)    ] [    X (0..127)    ]
```

### 3.3 Linear Address Calculation

To compute the memory address for a given pixel $(X, Y)$ within the `$2000` base window:

$$\text{Effective Address} = \$2000 + (Y \ll 7) + X = \$2000 + (Y \times 128) + X$$

#### Memory Boundary Considerations:
- **Rows 0 to 63:** $64 \times 128 = 8,192\text{ bytes}$ (`$0000`–`$1FFF` offset). These rows fit entirely within the 8 KB MMIO window (`$2000`–`$3FFF`).
- **Rows 64 to 74:** Require offset $64 \times 128 = \$2000$, placing them at CPU address `$4000` and above. In systems using the standard `GPU`, accessing rows 64–74 directly over the bus requires bank switching or specialized memory layout configurations.
- **microDOS-GPU Advantage:** In `MicroDosGPU`, writes outside the visible $100 \times 75$ bounds fall directly into `extraRam` (`0x0000`–`0x1FFF`), allowing arbitrary auxiliary storage.

---

## 4. Pixel Format & Color Palettes

### 4.1 64-Color RGB 2:2:2 Format

Every byte in VRAM represents exactly one visible pixel packed into an **RGB 2:2:2** format (6 bits used, top 2 bits ignored on write):

```
Bit:    7   6   5   4   3   2   1   0
      +---+---+---+---+---+---+---+---+
      | - | - |  RED  | GREEN | BLUE  |
      +---+---+---+---+---+---+---+---+
               [ 2b ]  [ 2b ]  [ 2b ]
```

| Channel | Bits | Values | Intensity Mapping |
|:---|:---:|:---:|:---|
| **Red** | `[5:4]` | `00`–`11` | $0 \to 0$, $1 \to 85$ (`0x55`), $2 \to 170$ (`0xAA`), $3 \to 255$ (`0xFF`) |
| **Green** | `[3:2]` | `00`–`11` | $0 \to 0$, $1 \to 85$ (`0x55`), $2 \to 170$ (`0xAA`), $3 \to 255$ (`0xFF`) |
| **Blue** | `[1:0]` | `00`–`11` | $0 \to 0$, $1 \to 85$ (`0x55`), $2 \to 170$ (`0xAA`), $3 \to 255$ (`0xFF`) |

#### OpenGL / Host Color Conversion:
```cpp
// Conversion logic in VRAMImageIO.cpp & VRAMViewerWindow.cpp
uint8_t r = ((val >> 4) & 0x03) * 85;
uint8_t g = ((val >> 2) & 0x03) * 85;
uint8_t b = (val & 0x03) * 85;
```

### 4.2 Standard 64-Color vs. Console 256-Color Palette

The GPU's 64-color palette is intentionally distinct from the ANSI 256-color palette used by the emulator's VT100 Console subsystem:

- **GPU 64-color (`64-color.act`):** 6-bit direct digital color with uniform $4 \times 4 \times 4$ color cube quantization. Adobe Color Table (`tools/gpu/64-color.act`) is provided for Photoshop/GIMP asset authoring.
- **Console 256-color:** Standard 8-bit xterm/ANSI palette containing 16 standard colors, a $6 \times 6 \times 6$ RGB cube (indices 16–231), and 24 grayscale steps (indices 232–255).

### 4.3 Asset Conversion Pipeline (`image-to-bin.py`)

The project includes an automated Python conversion tool at `tools/gpu/image-to-bin.py` to convert common image formats (PNG, JPG, BMP) into raw binary VRAM dumps:

```bash
python3 tools/gpu/image-to-bin.py assets/vram/splash.png
# Generates output/splash.bin (9,600 bytes = 75 lines * 128 stride)
```

The conversion script automatically scales the input image to $100 \times 75$ pixels using Lanczos filtering, quantizes each color channel to 2 bits ($0 \dots 3$), and packs pixels using a 128-byte line stride.

---

## 5. Raster Timing Generator & CPU Wait States

### 5.1 Scanning Engine (`RasterTimingGenerator`)

The GPU advances its internal electron beam counter once per system clock tick via `GPU::Clock()`:

1. **Active Scanline:** For lines $Y \in [0, 63]$, pixels $X \in [0, 99]$ are actively drawn (`isBlanking = false`).
2. **Horizontal Blanking (H-Blank):** When $X$ reaches 100, `isBlanking` asserts `true` for 32 clock cycles until $X = 132$. The beam then resets to $X = 0$ and increments $Y$.
3. **Vertical Blanking (V-Blank):** When $Y$ advances to `VRAM_HEIGHT_DRAWABLE_BY_CPU` (row 64), `isYDrawing` becomes `false` and `isBlanking` remains `true` continuously for lines 64 through 77.
4. **Frame Retrace:** When $Y$ reaches `DISPLAY_HEIGHT` (line 78), $Y$ resets to 0 and active drawing resumes.

### 5.2 CPU Wait States (`BlocksCPU()`)

Real CRT hardware prevents CPU bus writes while the electron gun accesses VRAM memory to prevent visual "snow" and bus contention:

- **Standard `GPU::BlocksCPU()` returns `true`:**
  In `Emulator::RunCPUTick`, if `activeGpu->BlocksCPU()` is enabled, the 65c02 CPU is stalled during active drawing periods ($X < 100$ on rows $0 \dots 63$). The CPU only steps instructions when `activeGpu->IsInBlankingInterval()` is `true`.
- **`MicroDosGPU::BlocksCPU()` returns `false`:**
  Under the microDOS graphical desktop, CPU wait states are bypassed to maximize GUI responsiveness.

```cpp
// Core execution loop in Emulator.cpp
if (gpuEnabled && activeGpu->BlocksCPU()) {
    activeGpu->Clock();
    if (activeGpu->IsInBlankingInterval()) {
        res = cpu.StepFast<Debug>(bus);
        cpuStepped = true;
    }
} else {
    res = cpu.StepFast<Debug>(bus);
    cpuStepped = true;
}
```

### 5.3 Slow-Motion Beam Emulation

The emulator includes a slow-motion beam debug mode (`rasterBeamSlowMotion`). When active, CPU wait states are relaxed so developers can observe the raster beam sweeping across the display in real time within the **VRAM Viewer Window**:

- **Cyan Dot & Scanline Trail:** Real-time location of $(X, Y)$ within visible area.
- **Orange Indicator:** Beam traveling through the 32-cycle H-Blank interval.
- **Red Retrace Vector:** Vertical retrace traversing from bottom-right back to top-left.
- **Status HUD:** Real-time display showing `BEAM: (X, Y) [DRAWING|H-BLANK|V-BLANK]`.

---

## 6. VRAM Dirty Tracking & Rendering Pipeline

To sustain 60+ FPS without draining host CPU/GPU bandwidth, the GPU implements lock-free atomic dirty tracking:

```cpp
mutable std::atomic<bool> vramDirty{true};

void Write(Word addr, Byte val) {
    Byte x = addr & 0x7F;
    Byte y = (addr >> 7) & 0x7F;
    if (x < VRAM_WIDTH && y < VRAM_HEIGHT) {
        vram[y][x] = val;
        vramDirty.store(true, std::memory_order_relaxed);
    }
}
```

The host GUI checks `gpu.ConsumeVRAMDirty()` at the beginning of each frame. If `false`, host texture re-upload is skipped entirely. If `true`, `VRAMImageIO::ConvertVRAMToRGB()` converts the raw 64-color bytes into an uncompressed 24-bit RGB pixel buffer ($100 \times 75 \times 3$ bytes) and streams it via `glTexSubImage2D` directly into an OpenGL 2D texture.

---

## 7. microDOS-GPU & Hardware PS/2 Mouse

When operating with microDOS cartridges, the system instantiates `MicroDosGPU` instead of the base `GPU`:

1. **Auxiliary RAM:** Addresses beyond visible pixels ($X \ge 100$ or $Y \ge 75$) up to 8 KB (`0x2000`) access `extraRam`.
2. **Hardware Mouse Controller (`PS2MouseController`):** An integrated PS/2 state machine converts host mouse movements into hardware packets.
3. **Blanking Burst Transfer:** During the blanking interval, the virtual FPGA reads the PS/2 mouse registers and bursts the coordinate state directly into fixed offsets within the GPU address space:
   - Offset `+$1FF0`: Mouse Buttons (Bit 0 = Left, Bit 1 = Right, Bit 2 = Middle)
   - Offset `+$1FF1`: Mouse X position ($0 \dots 99$)
   - Offset `+$1FF2`: Mouse Y position ($0 \dots 74$)

Depending on the bus mapping:
- **microDOS Layout (Base `$5FC0`):** Addresses are **`$7FB0`**, **`$7FB1`**, and **`$7FB2`** (defined in `Libs/Mouse.h` via `MOUSE_REG_BTNS_ADDR`).
- **Default Standalone Layout (Base `$2000`):** Addresses are **`$3FF0`**, **`$3FF1`**, and **`$3FF2`**.

Software can poll mouse coordinates directly from memory without requiring an interrupt service routine!

---

## 8. Programming Examples

### 8.1 65c02 Assembly: Plotting a Pixel

```assembly
; =====================================================================
; PlotPixel: Plots a pixel at (X, Y) with Color in Accumulator
; Input:
;   $00 = X coordinate (0..99)
;   $01 = Y coordinate (0..74)
;   A   = RGB 2:2:2 Color byte (%00RRGGBB)
; =====================================================================
VRAM_BASE   = $2000

PlotPixel:
    PHA                 ; Preserve color
    LDA $01             ; Load Y
    LSR                 ; Shift bit 0 of Y into Carry
    ROR $02             ; Rotate into High byte of offset
    ; Y coordinate is shifted left by 7:
    ; addr_low  = X | ((Y & 1) << 7)
    ; addr_high = (Y >> 1) + High(VRAM_BASE)
    LDA $00             ; Load X
    BCC @NoCarry
    ORA #$80            ; Set bit 7 if Y bit 0 was set
@NoCarry:
    STA $02             ; Low byte of pointer

    LDA $01             ; Load Y again
    LSR                 ; Y >> 1
    CLC
    ADC #>$VRAM_BASE    ; Add $20
    STA $03             ; High byte of pointer

    PLA                 ; Restore color
    LDY #0
    STA ($02),Y         ; Write pixel directly to VRAM
    RTS
```

### 8.2 C Example: Clearing the Screen and Drawing a Box

```c
#include <stdint.h>

#define VRAM_BASE ((volatile uint8_t*)0x2000)
#define VRAM_WIDTH  100
#define VRAM_HEIGHT 75
#define VRAM_STRIDE 128

// Color definitions (RGB 2:2:2)
#define COLOR_BLACK 0x00
#define COLOR_RED   0x30  // R=3, G=0, B=0
#define COLOR_GREEN 0x0C  // R=0, G=3, B=0
#define COLOR_BLUE  0x03  // R=0, G=0, B=3
#define COLOR_WHITE 0x3F  // R=3, G=3, B=3

void gpu_put_pixel(uint8_t x, uint8_t y, uint8_t color) {
    if (x < VRAM_WIDTH && y < VRAM_HEIGHT) {
        // Effective address: 0x2000 + (y << 7) + x
        VRAM_BASE[(y << 7) | x] = color;
    }
}

void gpu_clear(uint8_t color) {
    for (uint8_t y = 0; y < 64; ++y) {
        volatile uint8_t* row = &VRAM_BASE[y << 7];
        for (uint8_t x = 0; x < VRAM_WIDTH; ++x) {
            row[x] = color;
        }
    }
}

void gpu_draw_rect(uint8_t x0, uint8_t y0, uint8_t w, uint8_t h, uint8_t color) {
    for (uint8_t y = y0; y < y0 + h && y < 64; ++y) {
        for (uint8_t x = x0; x < x0 + w && x < VRAM_WIDTH; ++x) {
            gpu_put_pixel(x, y, color);
        }
    }
}
```
