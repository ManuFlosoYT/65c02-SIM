# SDK Example Programs Catalog

The `sdk/src/` directory contains complete reference programs written in **C23** and **65C02 Assembly**. These programs demonstrate the full hardware capabilities of the 65c02-SIM platform, including 3D graphics rendering, double-buffered video, 3-voice SID audio synthesis, FAT16 SD card file access, ESP8266 networking, and VIA timer interrupts.

All programs are compiled using **LLVM-MOS** (`compile-bin.sh`) and packaged into `.65c` cartridges ready to run in the simulator.

---

## 1. Catalog of Example Programs

### 1. `cube.c` — Real-Time 3D Rotating Wireframe Cube
- **Hardware Used:** GPU Double Buffering (`Libs/GPUDoubleBuffer.h`), VIA 6522 (`Libs/VIA.h`), LCD (`Libs/LCD.h`), ACIA BIOS (`Libs/BIOS.h`).
- **Description:** Renders a fully rotating 3D wireframe cube in real time at 30–60 FPS.
- **Implementation Details:**
  - Employs an 8-bit fixed-point lookup table (`SIN_TABLE[256]`) for trigonometric sine and cosine calculations without floating-point overhead.
  - Implements 3D rotation matrix transformations around the X, Y, and Z axes.
  - Uses `GPUDoubleBuffer.h` to draw into the back buffer (`$6010–$790F`) and blit cleanly to the front buffer (`$2000–$3FFF`) during vertical blanking, eliminating visual screen tearing.
  - VIA 6522 Timer 1 is configured in continuous mode to pace frame timing precisely.
- **Compilation:**
  ```bash
  ./sdk/scripts/compile-bin.sh cube
  ```

---

### 2. `room.c` — 3D Room Engine
- **Hardware Used:** GPU Double Buffering (`Libs/GPUDoubleBuffer.h`), LCD (`Libs/LCD.h`).
- **Description:** An interactive real-time 3D camera engine rendering a wireframe architectural room model consisting of 21 vertices and 30 connected polygon faces.
- **Implementation Details:**
  - Employs an 8-bit fixed-point lookup table (`SIN_TABLE[256]`) for sine and cosine trigonometric functions (`ISIN` and `ICOS`), avoiding costly runtime floating-point operations.
  - Transforms 3D world coordinates $(X, Y, Z)$ into camera-relative space based on viewer position and yaw rotation angle.
  - Perspective projection projects 3D coordinates onto 2D screen coordinates $(X', Y')$ scaled to the 100x75 pixel display resolution.
  - Employs double buffering (`Libs/GPUDoubleBuffer.h`) to render into offscreen back-buffer RAM ($6010–$790F) and blit cleanly to front-buffer VRAM ($2000–$3FFF), preventing flicker.
  - Recommended CPU clock frequency: 10–100 MHz (`--ips 100000000`) for fluid navigation.
- **Compilation:**
  ```bash
  ./sdk/scripts/compile-bin.sh room
  ```

---

### 3. `game.c` — 2D Platformer Game
- **Hardware Used:** GPU (`Libs/GPU.h`), VIA 6522 (`Libs/VIA.h`), LCD (`Libs/LCD.h`), ACIA BIOS (`Libs/BIOS.h`).
- **Description:** An interactive 2D side-scrolling platformer demonstrating a classic game loop: input processing, physical simulation, collision detection, and sprite rendering.
- **Implementation Details:**
  - Character physics: gravity acceleration, variable jump velocity, horizontal inertia, and bounding-box collision against multi-tiered platforms.
  - VIA 6522 Timer 1 generates steady 60 Hz interrupts for physics updates.
  - Controls via serial console keys (`A` = Left, `D` = Right, `W` or `Space` = Jump).
- **Compilation:**
  ```bash
  ./sdk/scripts/compile-bin.sh game
  ```

---

### 4. `tetris.c` — Full-Featured Tetris Game
- **Hardware Used:** GPU (`Libs/GPU.h`), SID 6581/8580 (`Libs/SID.h`), VIA 6522 (`Libs/VIA.h`), LCD (`Libs/LCD.h`), BIOS.
- **Description:** Complete implementation of the classic Tetris game featuring color graphics, score tracking, high score records, upcoming piece preview, and SID sound effects.
- **Implementation Details:**
  - 10x18 grid array with 3x3 pixel block rendering.
  - 7 tetromino shapes (`I`, `O`, `T`, `S`, `Z`, `J`, `L`) with 4-phase rotation matrices (`SHAPES[7][4][4][2]`).
  - Line clearing animation and level progression.
  - Sound effects generated via `Libs/SID.h` for piece movement, rotation, line clear, and game over.
- **Compilation:**
  ```bash
  ./sdk/scripts/compile-bin.sh tetris
  ```

---

### 5. `mat.c` & `mat64.c` — Integer Matrix Multiplication Benchmarks
- **Hardware Used:** ACIA BIOS (`Libs/BIOS.h`).
- **Description:** CPU computational benchmarks measuring arithmetic throughput on 8-bit and 64-bit multi-precision integer matrix operations.
- **Implementation Details:**
  - `mat.c`: 8-bit integer matrix multiplication measuring raw loop efficiency and array indexing performance.
  - `mat64.c`: 64-bit wide integer operations emulated in software, demonstrating LLVM-MOS compiler synthesis of 64-bit mathematical helper routines on an 8-bit accumulator.
- **Compilation:**
  ```bash
  ./sdk/scripts/compile-bin.sh mat
  ./sdk/scripts/compile-bin.sh mat64
  ```

---

### 6. `echo.c` — Minimal Serial Echo Template
- **Hardware Used:** ACIA 6850 BIOS (`Libs/BIOS.h`).
- **Description:** The canonical starting template for new 65C02 projects. Reads characters received over the serial interface and echoes them back to the terminal console.
- **Source:**
  ```c
  #include "Libs/BIOS.h"

  int main(void) {
      INIT_BUFFER();
      println("Echo program ready. Type characters:");
      while (1) {
          char c = bios_getchar();
          bios_putchar(c);
      }
      return 0;
  }
  ```
- **Compilation:**
  ```bash
  ./sdk/scripts/compile-bin.sh echo
  ```

---

### 7. `testNET.c` — ESP8266 WiFi & TCP HTTP Client
- **Hardware Used:** ESP8266 WiFi UART (`Libs/NET.h`), ACIA BIOS (`Libs/BIOS.h`).
- **Description:** Demonstrates internet connectivity by connecting to a WiFi network, opening a TCP socket, and performing an HTTP `GET` request.
- **Implementation Details:**
  - Sends Hayes AT commands to the ESP8266 UART:
    1. `AT+CWJAP="EmulatorNet","password"` (join access point)
    2. `AT+CIPSTART="TCP","www.google.com",80` (open socket)
    3. `AT+CIPSEND=40` (transmit raw HTTP request)
  - Reads returning TCP stream byte-by-byte via `net_getc()` and streams the response to the console.
- **Compilation:**
  ```bash
  ./sdk/scripts/compile-bin.sh testNET
  ```

---

### 8. `testSD.c` — FAT16 Storage & FatFs Verification
- **Hardware Used:** SD Card SPI Controller, FatFs library (`Libs/SD.h`), ACIA BIOS.
- **Description:** Full-stack integration test for disk storage:
  1. Mounts the attached FAT16 SD card image (`sd_mount()`).
  2. Creates and writes text to `TEST.TXT` (`sd_open`, `sd_puts`, `sd_close`).
  3. Reopens `TEST.TXT` in read mode and verifies file contents (`sd_getc`).
  4. Scans and prints the root directory listing (`sd_opendir`, `sd_readdir`).
  5. Unmounts the card cleanly (`sd_unmount()`).
- **Compilation:**
  ```bash
  ./sdk/scripts/compile-bin.sh testSD
  ```

---

### 9. `testSID.c` — 3-Voice Polyphonic Sound Synthesis
- **Hardware Used:** MOS 6581/8580 SID (`Libs/SID.h`).
- **Description:** Exercises all three hardware sound voices simultaneously:
  - Voice 1: Leads melody using Pulse waveform with pulse-width modulation (`0x800`).
  - Voice 2: Bass accompaniment using Sawtooth waveform.
  - Voice 3: Percussion rhythm using White Noise waveform with rapid attack/decay envelope.
- **Compilation:**
  ```bash
  ./sdk/scripts/compile-bin.sh testSID
  ```

---

### 10. `testGPU.c` — 2D Drawing Primitives Test
- **Hardware Used:** GPU (`Libs/GPU.h`), ACIA BIOS.
- **Description:** Validates GPU rasterization routines:
  - Solid screen fill with color (`gpu_fill_screen`).
  - Outlined and filled rectangles (`gpu_draw_rect`).
  - Bresenham line rasterization (`gpu_draw_line`).
  - Solid and wireframe triangles (`gpu_draw_tri`).
  - Built-in hardware test color bars (`gpu_test_pattern`).
- **Compilation:**
  ```bash
  ./sdk/scripts/compile-bin.sh testGPU
  ```

---

### 11. `vga.s` — Pure Assembly GPU Raster Driver
- **Hardware Used:** GPU VRAM (`$2000–$3FFF`).
- **Description:** Demonstrates direct hardware access in pure 65C02 assembly. Continuously increments a color value and fills all 32 pages ($2000 bytes) of VRAM with an animated color cycle.
- **Syntax:** LLVM-MOS / GNU Assembler syntax (`.section .text`, `.globl reset`).
- **Compilation:**
  ```bash
  ./sdk/scripts/compile-bin.sh vga
  ```

---

### 12. `wozmon.s` — Classic Apple 1 Monitor Assembly
- **Hardware Used:** ACIA UART (`$5000`), RAM (`$0200` buffer).
- **Description:** Authentic assembly source of Steve Wozniak's Apple 1 monitor, adapted to run on the 65c02-SIM ACIA serial port.
- **Compilation:**
  ```bash
  ./sdk/scripts/compile-bin.sh eater
  ```

---

### Additional Test Utilities
- `test.c`: Minimal starter test program printing a Hello World string over the BIOS console.
- `fillVRAM.c`: Measures the maximum pixel fill throughput of the GPU.
- `fullTest.c`: Comprehensive burn-in test exercising CPU instructions, RAM integrity, ACIA, VIA, and timers simultaneously.
- `testVIA.c`: Tests VIA 6522 Timer 1 continuous mode, Timer 2 one-shot mode, and input/output port pins.

---

## 2. Compilation, Flashing & Packaging

### Compiling to Flat ROM Binary
Run `compile-bin.sh` with the target name:
```bash
./sdk/scripts/compile-bin.sh cube
```
This produces:
- `output/rom/cube.bin`: Raw 32 KB binary loaded at `$8000`.
- `output/rom/cube.elf`: ELF executable containing full DWARF debug symbols.

### Packaging into a `.65c` Cartridge
To create a complete cartridge with hardware configurations:
```bash
./sdk/scripts/create-cartridge.sh output/rom/cube.bin \
    --name "3D Cube Demo" \
    --author "65c02-SIM SDK" \
    --gpu true \
    --ips 2000000
```
The output file `output/cartridge/cube.65c` can be loaded into the simulator directly via drag-and-drop or the GUI File menu.

---

## 3. Debugging SDK Programs

Every compilation with LLVM-MOS produces an ELF binary with DWARF debug info (`output/rom/<name>.elf`).

### Using Debug Symbols in the Simulator
1. Load `output/rom/<name>.bin` (or `<name>.65c`).
2. Open the **Debugger Window** in the simulator GUI.
3. The simulator automatically matches function names, variable addresses, and source line numbers from the companion `.elf` file.
4. Set hardware or software breakpoints on C function names (e.g. `break main`, `break render_cube`).
5. Step through source code line-by-line while inspecting zero-page registers `__rc0`–`__rc31`, CPU registers (`A`, `X`, `Y`, `SP`, `PC`), and MMIO states.