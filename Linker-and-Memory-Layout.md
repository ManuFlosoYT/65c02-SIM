# Linker & Memory Layout Reference

This document provides a comprehensive technical reference for the system memory layout and the **LLVM-MOS** GNU linker scripts used in 65c02-SIM.

The project uses modern LLVM-MOS (`mos-clang` and GNU `ld.lld`), completely replacing legacy cc65/ld65 configuration files with standard GNU linker scripts (`link.ld`), Link-Time Optimization (LTO), and dynamic memory map generation.

---

## 1. System Memory Architecture

The 65C02 processor addresses a flat 64 KB (65,536 bytes) physical address space (`$0000–$FFFF`). The simulator supports two primary memory architectures:

1. **Standard Mode:** Used for standalone SDK ROM programs, demos, and firmware.
2. **microDOS Mode:** Used when executing the microDOS operating system and its `.app` binaries.

```
Standard Mode                          microDOS Mode
+---------------------------------+    +---------------------------------+
| $0000-$00FF: Zero Page          |    | $0000-$005F: App Zero Page      |
| $0100-$01FF: CPU Hardware Stack |    | $0060-$0062: Process Arguments  |
| $0200-$02FF: (Free / Temp)      |    | $0078-$00FB: Kernel Zero Page   |
| $0300-$03FF: BIOS Input Buffer  |    | $0100-$01FF: CPU Hardware Stack |
| $0400-$1FFF: RAM (General C)    |    | $0200-$53FF: App Safe RAM       |
| $2000-$3FFF: GPU VRAM (Optional)|    |   ($2800-$47FF: Overlay Window) |
| $4000-$47FF: RAM                |    | $5400-$5FBF: Kernel RAM & Stack |
| $4800-$481F: SID Registers      |    | $5FC0-$7FBF: microDOS-GPU VRAM  |
| $4820-$4FFF: RAM                |    |   ($7FB0-$7FB2: Mouse Registers)|
| $5000-$5003: ACIA UART          |    | $7FC0-$7FFF: Peripheral MMIO    |
| $5004-$5007: ESP8266 WiFi       |    |   $7FC0-$7FC3: ACIA UART        |
| $5008-$500B: SD Card SPI        |    |   $7FC4-$7FC7: ESP8266 WiFi     |
| $6000-$600F: VIA 6522           |    |   $7FC8-$7FCB: SD Card SPI      |
| $6010-$790F: VRAM Back Buffer   |    |   $7FD0-$7FDF: VIA 6522         |
| $7910-$7FFF: RAM (Top of Stack) |    |   $7FE0-$7FFF: MOS 6581/8580 SID|
+---------------------------------+    +---------------------------------+
| $8000-$FFFF: 32 KB ROM Area     |    | $8000-$FFFF: microDOS Kernel ROM|
|   $FFFA-$FFFF: Hardware Vectors |    |   $FF90-$FFF9: ABI Jump Table   |
+---------------------------------+    |   $FFFA-$FFFF: Hardware Vectors |
                                       +---------------------------------+
```

---

## 2. Memory Maps Compared

### Standard Mode Memory Map

| Address Range | Size | Component | Function / Description |
|---|---|---|---|
| `$0000–$00FF` | 256 B | Zero Page | Fast-access memory. `__rc0`–`__rc31` reserved for LLVM-MOS virtual registers; remaining for C pointers/globals. |
| `$0100–$01FF` | 256 B | CPU Stack | 65C02 hardware call stack (`JSR`, `RTS`, `PHA`, `PLA`, interrupts). |
| `$0200–$02FF` | 256 B | Free RAM | General workspace. |
| `$0300–$03FF` | 256 B | BIOS Input Buffer | Circular FIFO buffer for incoming serial characters. |
| `$0400–$7FFF` | ~31 KB | System RAM | Main memory for C heap, global/static variables, and software stack. Split into contiguous blocks around MMIO. |
| `$2000–$3FFF` | 8 KB | GPU VRAM | 100x75 pixel display buffer (when GPU is enabled). |
| `$4800–$481F` | 32 B | SID 6581/8580 | 3-voice sound chip registers (when SID is enabled). |
| `$5000–$5003` | 4 B | ACIA 6850 | Serial console UART registers. |
| `$5004–$5007` | 4 B | ESP8266 WiFi | WiFi network co-processor MMIO. |
| `$5008–$500B` | 4 B | SD Card SPI | Hardware SPI controller registers. |
| `$6000–$600F` | 16 B | VIA 6522 | Versatile Interface Adapter (timers, LCD data port, handshakes). |
| `$6010–$790F` | ~6.4 KB| GPU Back Buffer | Secondary VRAM buffer (when double buffering is enabled). |
| `$8000–$FFFF` | 32 KB | ROM Area | Program executable code (`.text`), constants (`.rodata`), and vectors. |
| `$FFFA–$FFFF` | 6 B | CPU Vectors | Hardware NMI (`$FFFA`), RESET (`$FFFC`), and IRQ/BRK (`$FFFE`). |

---

### microDOS Mode Memory Map

| Address Range | Size | Component | Function / Description |
|---|---|---|---|
| `$0000–$005F` | 96 B | App Zero Page | Dedicated to user `.app` binaries (`__rc0`–`__rc31` + app globals). |
| `$0060–$0062` | 3 B | Process Arguments| `$60` = `arg_count`, `$61–$62` = 16-bit pointer to `args[]` string array. |
| `$0078–$00FB` | 132 B | Kernel Zero Page | Internal microDOS kernel pointers, FatFs structures, and state. |
| `$00FC–$00FD` | 2 B | UART Pointers | BIOS circular buffer `READ_PTR` and `WRITE_PTR`. |
| `$0100–$01FF` | 256 B | CPU Stack | Hardware return stack. |
| `$0200–$53FF` | 20.5 KB| App Safe RAM | Primary executable space for `.app` programs. Program loaded at `$0200`. App software stack top is `$53FF`. |
| `$2800–$47FF` | 8 KB | Overlay Window | Virtual memory window. Paged `.app` overlay banks are loaded here on demand. |
| `$5400–$5FBF` | 3008 B | Kernel RAM | Operating system workspaces, disk buffers, and kernel stack (`__stack = 0x5FBF`). |
| `$5FC0–$7FBF` | 8 KB | microDOS-GPU VRAM| 100x75 VRAM @ 1 byte/pixel (RGB 222). When GPU is off, acts as 8KB extended app heap (`APP_EXTRA_RAM`). |
| `$7FB0–$7FB2` | 3 B | Host Mouse | Hardware mouse registers: Buttons (`$7FB0`), X (`$7FB1`), Y (`$7FB2`). Within `MicroDosGPU`, these reside at internal offsets `$1FF0`, `$1FF1`, `$1FF2`. |
| `$7FC0–$7FFF` | 64 B | MMIO Area | Re-mapped peripheral space: ACIA (`$7FC0`), ESP8266 (`$7FC4`), SD (`$7FC8`), VIA (`$7FD0`), SID (`$7FE0`). |
| `$8000–$FFFF` | 32 KB | microDOS ROM | Monolithic kernel, FatFs drivers, and filesystem routines. |
| `$FF90–$FFF9` | 106 B | ABI Jump Table | 35 fixed 3-byte indirect jumps (`JMP <func>`) to kernel routines. |
| `$FFFA–$FFFF` | 6 B | CPU Vectors | Hardware NMI, RESET, IRQ vectors. |

---

## 3. LLVM-MOS Linker Script Architecture

The toolchain uses GNU linker scripts compliant with LLVM-MOS. The base script is [`sdk/linker/link.ld`](file:///home/manu/Documents/GitHub/65c02-SIM/sdk/linker/link.ld):

```ld
/* Linker script base for 65c02-SIM with LLVM-MOS */

__rc0 = 0x00;
INCLUDE imag-regs.ld
ASSERT(__rc0 == 0x00, "Error in Zero Page mapping.")
ASSERT(__rc31 == 0x1F, "Error in Zero Page mapping.")

MEMORY {
    zp           (rw) : ORIGIN = __rc31 + 1, LENGTH = 0x100 - (__rc31 + 1)
    input_buffer (rw) : ORIGIN = 0x0300,     LENGTH = 0x0100
    ram          (rw) : ORIGIN = 0x0400,     LENGTH = 0x7C00
    rom          (rx) : ORIGIN = 0x8000,     LENGTH = 0x8000
}

REGION_ALIAS("c_readonly", rom)
REGION_ALIAS("c_writeable", ram)

SECTIONS {
    .input_buffer 0x0300 (NOLOAD) : {
        KEEP(*(SORT_BY_NAME(.input_buffer*)))
        *(.input_buffer)
    } > input_buffer

    INCLUDE c.ld

    .vectors 0xFFFA : {
        SHORT(DEFINED(_nmi) ? _nmi : 0x0F00)
        SHORT(DEFINED(_start) ? _start : (DEFINED(_reset) ? _reset : 0x8000))
        SHORT(DEFINED(IRQ_HANDLER) ? IRQ_HANDLER : (DEFINED(_irq) ? _irq : 0))
    } > rom
}

__stack = 0x7FFF;

OUTPUT_FORMAT {
    FULL(rom)
}
```

### Standard Linker Sections
- `.text`: Machine instructions and executable code (placed in `rom` or `app_ram`).
- `.rodata`: Read-only constants, string literals, and lookup tables (placed in `rom`).
- `.data`: Initialized global and static variables. Copied from ROM to RAM at startup by `__do_copy_data`.
- `.bss`: Uninitialized global and static variables. Zeroed at startup by `__do_zero_bss`.
- `.zeropage` / `.zp`: Fast-access variables placed directly in Zero Page.
- `.input_buffer`: Fixed at `$0300` for BIOS serial reception (`NOLOAD`).
- `.vectors`: Fixed at `$FFFA` for 6502 hardware interrupts.

### Zeropage Imaginary Registers (`imag-regs.ld`)
The 6502 architecture provides only three 8-bit registers (`A`, `X`, `Y`). To compile high-level C efficiently, LLVM-MOS maps 32 virtual registers—known as **imaginary registers** (`__rc0` through `__rc31`)—into the Zero Page:
- `__rc0`–`__rc3`: Used for 16-bit/32-bit function return values and expression evaluation.
- `__rc4`–`__rc15`: General-purpose compiler scratch registers and pointer arithmetic.
- `__rc16`–`__rc31`: Register variables and callee-saved registers.

The linker script asserts that these registers reside between `$00` and `$1F`:
```ld
ASSERT(__rc0 == 0x00, "Error in Zero Page mapping.")
ASSERT(__rc31 == 0x1F, "Error in Zero Page mapping.")
```
User zero-page variables begin immediately following `__rc31` (`ORIGIN = __rc31 + 1`).

### Stack Architecture
- **Hardware Call Stack:** 256 bytes at `$0100–$01FF` managed by the 6502 `S` register. Used solely for subroutine call returns (`JSR`/`RTS`) and interrupt frames.
- **Software Data Stack:** Managed by LLVM-MOS virtual frame pointers (`__stack`). It grows downwards from the highest address in available RAM:
  - Standard programs: `__stack = 0x7FFF;`
  - microDOS apps: `__stack = 0x53FF;`
  - microDOS kernel: `__stack = 0x5FBF;`
- Initialized automatically via `-linit-stack` in the compiler flags.

---

## 4. Dynamic Linker Script Generation

To prevent memory collisions between user data and hardware MMIO regions, the SDK dynamically generates customized linker scripts at compile time.

### 1. `generate_link_ld.py` (Standard Programs & Kernel)
Located at [`tools/linker/generate_link_ld.py`](file:///home/manu/Documents/GitHub/65c02-SIM/tools/linker/generate_link_ld.py).
During compilation, `compile-bin.sh` inspects the program’s `#include` directives:
```bash
python3 tools/linker/generate_link_ld.py [--gpu] [--double-buffer] [--net] [--sid] [--microDOS]
```

**How it partitions memory:**
1. Collects all reserved peripheral address ranges (e.g. `$2000–$3FFF` for GPU, `$4800–$481F` for SID, `$5000–$5003` for ACIA).
2. Merges overlapping or adjacent regions.
3. Automatically carves the remaining RAM (`$0400–$7FFF`) into a sequence of safe, non-overlapping memory blocks (`ram_1`, `ram_2`, etc.).
4. Assigns `c_writeable` to `ram_1` and sets `__stack` to the top of the highest available RAM block.

---

### 2. `generate_app_link_ld.py` (microDOS Applications)
Located at [`tools/linker/generate_app_link_ld.py`](file:///home/manu/Documents/GitHub/65c02-SIM/tools/linker/generate_app_link_ld.py).
Generates linker scripts specifically for microDOS `.app` binaries:
```bash
python3 tools/linker/generate_app_link_ld.py [--gpu] [--overlays <N>]
```

**Key Features:**
1. **App Memory Boundaries:** Restricts application RAM to `$0200–$53FF`.
2. **Zero Page Boundaries:** Caps application zero-page symbols at `$005F` so they never collide with microDOS kernel zero-page variables (`$0078–$00FB`).
3. **ABI Jump Table Symbol Resolution:**
   Every system function is exported as an absolute linker symbol pointing to its fixed `$FF90 + offset` address:
   ```ld
   PROVIDE(print_str = 0xFF90);
   PROVIDE(println   = 0xFF93);
   PROVIDE(sd_open   = 0xFFA2);
   PROVIDE(sd_read   = 0xFFA5);
   ...
   ```
   When user code calls `println("Hi")`, LLVM-MOS emits a direct `jsr $FF93`, completely eliminating runtime dynamic linking overhead.

4. **Overlay Bank Generation:**
   When `--overlays <N>` is specified, the script defines overlay memory regions with virtual 24-bit bank origins:
   ```ld
   core_ram    (rw) : ORIGIN = 0x0200,   LENGTH = 0x2600
   overlay_1   (rx) : ORIGIN = 0x012800, LENGTH = 0x2000
   overlay_2   (rx) : ORIGIN = 0x022800, LENGTH = 0x2000
   ```
   Each overlay page is mapped to load at `$2800` (size `$2000` = 8 KB). Resident trampolines (`OVERLAY_TRAMPOLINE`) in [`sdk/linker/App-Runtime.s`](file:///home/manu/Documents/GitHub/65c02-SIM/sdk/linker/App-Runtime.s) automatically page the required bank into the `$2800` window via `os_load_app_page` before executing overlay code.
