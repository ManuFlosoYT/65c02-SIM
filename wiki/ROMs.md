# System Firmware & ROM Reference

The 65c02-SIM ecosystem features several system firmware configurations and ROM binaries designed for the 32 KB high-address window (`$8000–$FFFF`). This reference documents the internal architecture, memory layout, entry points, vector tables, and operation of each firmware image.

---

## 1. Firmware Overview & Memory Map

All ROM images occupy the upper 32 KB of the 65C02's address space. The last 6 bytes (`$FFFA–$FFFF`) are strictly reserved for the 6502 hardware interrupt vectors.

```
0x8000 +---------------------------------------------------------------+
       | Base ROM Firmware Area ($8000-$FDFF)                          |
       | - Ben Eater BIOS / Minimal Monitor OR                         |
       | - Microsoft BASIC (MSBASIC) OR                                |
       | - microDOS Monolithic Kernel OR                               |
       | - Native SIDLZ / NSF Music Player Core                        |
0xFE00 +---------------------------------------------------------------+
       | Monitor / BIOS High Region ($FE00-$FF8F)                      |
       | - WozMon System Monitor (Apple 1 Monitor)                     |
0xFF90 +---------------------------------------------------------------+
       | microDOS ABI Jump Table ($FF90-$FFF9) (microDOS firmware)     |
0xFFFA +---------------------------------------------------------------+
       | Hardware Interrupt Vectors ($FFFA-$FFFF)                      |
       | - $FFFA-$FFFB: NMI Vector                                     |
       | - $FFFC-$FFFD: RESET Vector                                   |
       | - $FFFE-$FFFF: IRQ / BRK Vector                               |
0xFFFF +---------------------------------------------------------------+
```

---

## 2. Ben Eater Minimal BIOS & Monitor

The standard hardware firmware provides a low-level BIOS for raw serial communication and basic I/O interfacing. Implemented in [`sdk/linker/bios.s`](file:///home/manu/Documents/GitHub/65c02-SIM/sdk/linker/bios.s), it serves as the foundation for standalone C and assembly SDK programs.

### Architecture
- **Console I/O:** Interfaces with an ACIA 6850 UART at `$5000` (Data `$5000`, Status `$5001`, Command `$5002`, Control `$5003`).
- **Circular Input Buffer:** A 256-byte ring buffer located in RAM at `$0300–$03FF`.
  - Pointers: `READ_PTR` and `WRITE_PTR` tracked in zero-page.
  - Asynchronous Interrupt Service Routine (`IRQ_HANDLER`): When the ACIA receives a byte, an interrupt triggers, reading the character from `$5000` and appending it to `$0300, X`.
- **Hardware Flow Control (RTS/CTS):**
  - Uses VIA 6522 Port A, Pin 0 (`PORTA`, `$6001`).
  - When the circular buffer reaches 240 bytes (`>= 0xF0`), the BIOS raises PA0 to throttle host serial input until the buffer is drained below 176 bytes (`< 0xB0`).

### Exported BIOS API

```asm
MONRDKEY / CHRIN    ; Read character from input buffer (blocking until key available)
MONGETCHAR_NB       ; Non-blocking character read (returns Carry clear if empty)
MONPEEK             ; Peek next character in buffer without consuming it
MONCOUT / CHROUT    ; Transmit character in accumulator A over ACIA UART
INIT_BUFFER         ; Initialize input buffer pointers and ACIA control registers
IRQ_HANDLER         ; Interrupt service routine for serial reception
```

### Serial Output Routine
```asm
MONCOUT:
    pha
.Lwait_tdre:
    lda ACIA_STATUS     ; Check if Transmitter Data Register is empty (bit 4)
    and #0x10
    beq .Lwait_tdre
    pla
    sta ACIA_DATA       ; Write character to serial bus
    rts
```

---

## 3. Microsoft BASIC for 6502 (MSBASIC)

The project includes an authentic port of the 1977 Microsoft BASIC 6502 interpreter (originally written by Bill Gates, Paul Allen, and Monte Davidoff), based on restorations by mist64 and Ben Eater.

### Memory Configuration
- **ROM Address Range:** `$8000–$FDFF` (approx. 31.5 KB)
- **Zero Page Allocation:**
  - `$0000–$001F`: Temporary registers, accumulator (`FAC1`), and floating point workspace.
  - `$0024–$002B`: WozMon state variables (shared).
  - `$007A–$007B`: `TXTPTR` — Pointer to currently executing BASIC line.
- **RAM Free Workspace:** `$0400–$7FFF` (approx. 31 KB for user BASIC programs and variable tables).

### Cold vs. Warm Start

| Entry Point | Address | Description |
|---|---|---|
| **Cold Start** | `$8000` | Full interpreter initialization. Clears all user programs, sets stack pointer, prints the memory size prompt (`MEMORY SIZE?`), detects top of RAM, prints `6502 BASIC` banner, and enters the interactive REPL. |
| **Warm Start** | `$8003` (or `RESTART`) | Re-enters BASIC without erasing user program memory. Restores execution pointers and resets the stack, preserving written lines. Useful after breaking to monitor or recovering from errors. |

### Tokenization & Execution
MSBASIC achieves high speed and compact memory usage through keyword tokenization:
- As lines are typed into the input buffer (`$0200`), the keyword scanner replaces full ASCII strings with 1-byte tokens ranging from `$80` to `$FF`:
  - `END` = `$80`, `FOR` = `$81`, `NEXT` = `$82`, `DATA` = `$83`, `INPUT` = `$84`
  - `DIM` = `$85`, `READ` = `$86`, `LET` = `$87`, `GOTO` = `$88`, `RUN` = `$89`
  - `IF` = `$8A`, `RESTORE` = `$8B`, `GOSUB` = `$8C`, `RETURN` = `$8D`, `REM` = `$8E`
  - `STOP` = `$8F`, `ON` = `$90`, `PRINT` = `$91`, `POKE` = `$92`, `WAIT` = `$93`
- Lines are stored as a linked list in RAM: each line begins with a 2-byte pointer to the next line, a 2-byte line number, followed by tokenized bytes and terminated with `$00`.

### Floating-Point Math
MSBASIC implements a 40-bit (5-byte) Microsoft Binary Format (MBF) floating-point engine:
- 1 byte signed exponent + 4 bytes fractional mantissa.
- Full transcendental math library: `SIN`, `COS`, `TAN`, `ATN`, `LOG`, `EXP`, `SQR`.

### I/O Vector Redirection & Zero-Page Routines
MSBASIC decouples the language interpreter core from physical peripherals through modular I/O vectors:

1. **Character Input (`CHRIN` / `MONRDKEY`):**
   - Located at `$A1F3` in `eater.bin` (or `$FFCF` in CBM builds).
   - Called by the input scanner whenever a line or character is requested (`INPUT`, `GET`).
   - Fetches incoming bytes from the 256-byte circular FIFO buffer at `$0300–$03FF`.
   - Automatically handles character echo via `CHROUT` and throttles hardware flow control (VIA Port A pin 0 RTS/CTS handshake).

2. **Character Output (`CHROUT` / `MONCOUT`):**
   - Located at `$A216` in `eater.bin` (or `$FFD2` in CBM builds).
   - Outputs the character in accumulator `A` to the ACIA 6850 serial UART at `$5000` (and mirrors output to the HD44780 LCD display if initialized).
   - Preserves all CPU registers (`PHA`/`PLA`).

3. **Zero-Page `CHRGET` / `CHRGOT` Scanner:**
   - On cold boot, MSBASIC copies a tiny, speed-critical 22-byte parser subroutine into the Zero Page at `$0073–$008A`:
     ```asm
     CHRGET:  inc TXTPTR      ; Increment 16-bit program pointer ($7A-$7B)
              bne CHRGOT
              inc TXTPTR+1
     CHRGOT:  lda (TXTPTR),y  ; Fetch current byte
              cmp #':'        ; Check statement separator
              bcs @not_sep
              ...
              rts
     ```
   - Residing in RAM allows advanced programs to hook or intercept token fetching to implement custom BASIC extensions.

4. **`USR` Machine Code Vector:**
   - Zero-page vector `USR` at `$0014` (or `$0000` depending on build) enables calling custom assembly subroutines from BASIC: `X = USR(Y)`.
   - Points to `IQERR` (Illegal Quantity error) by default; redirected by executing `POKE` statements targeting the vector address.

5. **Storage Vectors (`LOAD` & `SAVE`):**
   - Vectors in `sdk/msbasic/bios.s` stubbed with `rts`, providing standard entry hooks for attaching cassette or SD card disk storage drivers.

---

## 4. WozMon System Monitor

Located in high memory at `$FE00–$FFF9` in the `eater` ROM, **WozMon** is Steve Wozniak’s famous 256-byte machine language monitor for the original Apple 1 computer (1976). It allows developers to inspect, modify, and execute machine code directly through a serial terminal.

### Memory Location
- **Address:** `$FE00–$FFF9`
- **Zero Page Variables:**
  - `$24–$25`: `XAM` (Last examined memory location)
  - `$26–$27`: `ST` (Store destination pointer)
  - `$28–$29`: `L` / `H` (Hex parser buffer)
  - `$2B`: `MODE` (`$00` = Examine, `$7F` = Store, `$AE` = Block Examine)

### Command Syntax

```
\                   - Escape / cancel current command line buffer
<Address>           - Examine memory: Displays address and byte at location
                      Example: "3000" -> "3000: EA"
<Addr1>.<Addr2>     - Block Examine: Dumps memory between Addr1 and Addr2
                      Example: "3000.3010" -> Dumps 17 bytes
<Enter>             - Examine next 8 consecutive bytes
<Address>: <Bytes>  - Store memory: Writes byte list starting at Address
                      Example: "3000: A9 42 8D 00 50 60"
<Address>R          - Run: Executes machine code starting at Address
                      Example: "3000R"
```

### Typical Workflow: Launching BASIC from WozMon
When the system powers up into WozMon, you can start MSBASIC by jumping to its entry points:
```
\
8000R               ; Launch MSBASIC Cold Start (clears memory)
; OR
8003R               ; Launch MSBASIC Warm Start (preserves program)
```

---

## 5. microDOS Kernel ROM

The microDOS firmware ([`sdk/microdos/src/microDOS.c`](file:///home/manu/Documents/GitHub/65c02-SIM/sdk/microdos/src/microDOS.c)) boots the machine into a multi-tasking and GUI-capable desktop environment.

### Cold Boot Sequence
1. **Hardware Reset:** CPU reset vector fetches `$FFFC`, branching to `_reset` (`$8000`).
2. **CPU & Stack Setup:**
   - Clears decimal arithmetic flag (`cld`).
   - Software stack initialized to top of kernel RAM (`__stack = 0x5FBF`).
   - Enables interrupts (`cli`).
3. **Peripheral Initialization:**
   - Initializes ACIA 6850 UART at `$7FC0` to 19200 baud, 8 data bits, 1 stop bit, no parity.
   - Clears 2-line HD44780 LCD display (`lcd_init()`).
   - Clears virtual swap paging structures (`os_clear_swap_blocks()`).
4. **Storage Subsystem (SD & FAT16):**
   - SPI interface initialized via MMIO registers at `$7FC8–$7FCB`.
   - Mounts the SD card partition table and FAT16 filesystem (`sd_mount()`).
   - Verifies root directory `/` and system folder `/SYSTEM`.
5. **Desktop Autoloader:**
   - Looks for the graphical desktop binary at `/SYSTEM/DESKTOP/DESKTOP.SYS`.
   - Loads the desktop code into application safe RAM starting at `$0200`.
   - Calls the desktop entry point `DESKTOP_SHELL(line)`.
6. **Shell Fallback:**
   - If `DESKTOP.SYS` is absent or the user exits the GUI, the kernel drops into the interactive TUI command shell loop (`uDOS>`).

### Kernel Jump Table (`$FF90–$FFF9`)
The microDOS ROM exports 35 stable entry points (see [microDOS Manual](MicroDOS) for details) allowing application binaries to call operating system functions safely without hardcoding driver addresses.

---

## 6. NSF Player ROM

The NSF Player is a specialized firmware that turns the 65C02 simulator into a native NES chiptune player using the MOS 6581/8580 SID sound chip.

### Architecture & Operation
- **Source Pipeline:** Generated by [`tools/sid/audio_to_sid.py`](file:///home/manu/Documents/GitHub/65c02-SIM/tools/sid/audio_to_sid.py) and [`tools/sid/nsf_processor.py`](file:///home/manu/Documents/GitHub/65c02-SIM/tools/sid/nsf_processor.py).
- **Compression (SIDLZ):** NSF audio tracks contain high-density register write streams. The toolchain compiles these into a proprietary **SIDLZ** format combining semantic note-event deduplication with sliding-window LZ77 compression.
- **Decompression Ring Window:**
  - A 2 KB circular buffer (`$0200–$09FF`) in RAM.
  - The 65C02 player routine continuously streams compressed bytes from ROM, decompressing upcoming register updates just ahead of real-time playback.
- **VIA Timer Synchronization:**
  - VIA 6522 Timer 1 (`$6004`) is programmed for continuous interrupts at 60.098 Hz (NTSC frame rate).
  - Every vertical blanking tick advances the song playback position, calculates frequency sweeps, applies ADSR envelope changes, and writes to the SID hardware at `$4800` (or `$7FE0` in microDOS).
- **Shadow Registers:** Tracks all 25 SID registers in zero page to prevent unnecessary hardware bus writes and support polyphonic voice allocation.

---

## 7. Interrupt Vector Table ($FFFA–$FFFF)

Every ROM binary must supply the three mandatory 16-bit 65C02 hardware vector addresses at the top of memory:

```
+-----------+---------------+----------------------------------------------------+
| Address   | Vector Name   | Description                                        |
+-----------+---------------+----------------------------------------------------+
| $FFFA-$FFFB | NMI Vector    | Non-Maskable Interrupt (e.g., GPU V-Blank or FPGA) |
| $FFFC-$FFFD | RESET Vector  | Power-on / Hardware Reset Initial Program Counter  |
| $FFFE-$FFFF | IRQ / BRK     | Maskable Interrupt (ACIA UART RX, VIA Timer) & BRK |
+-----------+---------------+----------------------------------------------------+
```

### Vector Configuration Across Firmware Images

| Firmware Image | NMI (`$FFFA`) | RESET (`$FFFC`) | IRQ / BRK (`$FFFE`) |
|---|---|---|---|
| **Ben Eater / SDK Standard** | `$0F00` (stub) | `_start` (`$8000`) | `IRQ_HANDLER` (`bios.s`) |
| **MSBASIC + WozMon (`eater.bin`)** | `$0F00` (stub) | `RESET` (`$FE00` - WozMon) | `IRQ_HANDLER` (`bios.s`) |
| **microDOS Kernel** | `$0F00` (stub) | `_start` (`$8000`) | `IRQ_HANDLER` (`microdos_bios.s`)|
| **SIDLZ / NSF Player** | `$0000` (stub) | `_start` (`$8000`) | `timer_irq` (`play_loop`) |
