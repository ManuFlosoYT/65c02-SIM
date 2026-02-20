# Linker and Memory Layout

**Directory:** `Linker/`

## Overview

The `Linker/` directory contains the linker configuration files (`ld65` from the **cc65** suite), the system BIOS, and the Microsoft BASIC interpreter. These files define how code and data are laid out within the 65c02's 64 KB address space.

## Full memory map

```
0x0000 – 0x00FF   Zero Page — fast-access variables
0x0100 – 0x01FF   Stack
0x0200 – 0x02FF   (free)
0x0300 – 0x03FF   INPUT_BUFFER — BIOS circular input buffer
0x0400 – 0x47FF   RAM_1 — C variables and main heap
0x4800 – 0x481F   SID — 32 sound chip registers
0x4820 – 0x5FFF   RAM_2 — additional RAM
0x5000 – 0x5003   ACIA — serial communication registers (within RAM_2)
0x6000 – 0x600F   VIA — 16 I/O registers
0x6010 – 0x7FFF   RAM_3 — additional RAM
0x8000 – 0xFFF9   ROM — program code + BASIC + BIOS
0xFFFA – 0xFFFB   NMI vector
0xFFFC – 0xFFFD   RESET vector
0xFFFE – 0xFFFF   IRQ/BRK vector
```

> With the GPU enabled, `0x2000`–`0x3FFF` (within RAM_1) is used as VRAM.

## Linker configuration files

### `C-Runtime.cfg` — Standard C runtime

Default configuration for C programs. Defines:
- Zero page for cc65 runtime ZP variables
- C stack at `$7000`
- Code in ROM (`$8000`–`$FFFA`)
- Segments `DATA` (copied from ROM to RAM at startup), `BSS`, `CODE`, `RODATA`, `BIOS`

### `C-Runtime-GPU.cfg` — C runtime with GPU

Same as `C-Runtime.cfg` but reserves `0x2000`–`0x3FFF` as VRAM for the GPU. The C heap starts after the VRAM region.

### `C-Runtime-GPUDoubleBuffer.cfg` — GPU with double buffer

Same as `C-Runtime-GPU.cfg` but splits the VRAM into two halves for double-buffering (eliminates tearing in animations).

### `raw.cfg` — Pure assembly

Minimal configuration for assembly programs without a C runtime. Code starts directly at `$8000` and the interrupt vectors are at `$FFFA`–`$FFFF`.

## BIOS (`bios.s`)

The BIOS provides basic I/O routines used by both Microsoft BASIC and C programs through the cc65 runtime.

### Exported routines

| Symbol | Description |
|--------|-------------|
| `MONRDKEY` / `CHRIN` | Read a character from the input buffer (blocking) |
| `MONGETCHAR_NB` | Read a character from the buffer without blocking |
| `MONPEEK` | Peek at the next character without removing it |
| `MONCOUT` / `CHROUT` | Send a character via the ACIA (console) |
| `INIT_BUFFER` | Initialize the circular input buffer |
| `IRQ_HANDLER` | Interrupt handler (receives bytes via ACIA) |
| `LOAD` / `SAVE` | Empty stubs (for BASIC compatibility) |

### Circular input buffer

The BIOS implements a **256-byte circular buffer** at `$0300`–`$03FF`:
- `WRITE_PTR` (`$00`): write index (ISR → buffer)
- `READ_PTR` (`$01`): read index (program → buffer)
- The ISR (`IRQ_HANDLER`) writes each byte received via ACIA into the buffer
- `MONRDKEY` reads from the buffer in a blocking loop
- When the buffer is nearly full (`>= 0xB0`), hardware flow control is asserted (bit 0 of PORTA)

### Output routine `MONCOUT`

```asm
MONCOUT:
    pha
    sta ACIA_DATA       ; Send character over serial
    lda #$FF
@txdelay:
    dec                 ; Delay to simulate baud rate
    bne @txdelay
    pla
    rts
```

## C-Runtime (`C-Runtime.s`)

C runtime initialization code for cc65. Runs before `main()` and is responsible for:
1. Copying the `DATA` segment from ROM to RAM
2. Zeroing the `BSS` segment
3. Initializing the BIOS input buffer
4. Calling global C++ constructors
5. Calling `main()`

## WozMon (`wozmon.s`)

Apple 1 system monitor originally written by **Steve Wozniak** (© 1976 Apple Computer, Inc.). Allows inspecting and modifying memory directly from a serial console.

## Microsoft BASIC (`msbasic/`)

Directory containing the **Microsoft BASIC for 6502** port (© 1977 Microsoft), based on restorations by [mist64](https://github.com/mist64/msbasic) and [Ben Eater](https://github.com/beneater/msbasic).

- Build with `./msbasic/make.sh`
- The resulting binary is loaded into ROM as a sample program
- Allows writing BASIC programs directly in the emulator console

## Compilation tool

The cc65 suite is used to compile and link programs:

```bash
# Assemble
ca65 program.s -o program.o

# Link with the C runtime
ld65 -C Linker/C-Runtime.cfg program.o Linker/C-Runtime.o -o output/rom/program.bin
```

The `compile-bin.sh` script automates the entire process. See the [SDK](SDK) page for more details.
