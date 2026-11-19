# LCD — HD44780 16×2 Character Display Emulation

**Source Files:**
- Header: [`src/Hardware/Video/LCD.h`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Hardware/Video/LCD.h)
- Implementation: [`src/Hardware/Video/LCD.cpp`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Hardware/Video/LCD.cpp)
- Bus Latch: [`src/Hardware/Video/HD44780BusLatch.h`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Hardware/Video/HD44780BusLatch.h)
- Test Suite: [`src/UnitTests/LCD_Test.cpp`](file:///home/manu/Documents/GitHub/65c02-SIM/src/UnitTests/LCD_Test.cpp)

---

## 1. Overview

The emulator models a standard **16-column by 2-row alphanumeric liquid crystal display** controlled by the industry-standard **Hitachi HD44780** (or compatible) LCD controller. In hardware systems (such as Ben Eater's 6502 computer), the display is interfaced using the parallel I/O lines of a **MOS 6522 VIA (Versatile Interface Adapter)**.

In 65c02-SIM, the display is represented as a virtual bus device connected to VIA **Port B**. The simulation features cycle-accurate bus latching (`HD44780BusLatch`), support for 4-bit nibble transfers, display RAM (DDRAM) addressing, cursor positioning, and direct real-time rendering in the GUI's **LCD Window**.

---

## 2. Hardware Pinout & VIA Port B Interface

The LCD is wired directly to **Port B** of the 6522 VIA (`0x6000` in the default address map, or `0x7FD0` in microDOS / cartridge layouts):

```
             MOS 6522 VIA                          HD44780 LCD
           +--------------+                      +--------------+
           |       PB0    |--------------------->| DB4 (Pin 11) |
           |       PB1    |--------------------->| DB5 (Pin 12) |
           |       PB2    |--------------------->| DB6 (Pin 13) |
           |       PB3    |--------------------->| DB7 (Pin 14) |
           |       PB4    |--------------------->| RS  (Pin 4)  |
           |       PB5    |--------------------->| R/W (Pin 5)  |
           |       PB6    |--------------------->| E   (Pin 6)  |
           |       PB7    |                      |              |
           +--------------+                      +--------------+
```

### 2.1 Bitfield Breakdown (VIA Port B)

| Bit | Signal | Role | Logic Level Definition |
|:---:|:---:|:---|:---|
| **0** | `DB4` | Data Bus Bit 4 | Data / Instruction low bit of high nibble in 4-bit mode |
| **1** | `DB5` | Data Bus Bit 5 | Data / Instruction bit 1 |
| **2** | `DB6` | Data Bus Bit 6 | Data / Instruction bit 2 |
| **3** | `DB7` | Data Bus Bit 7 | Data / Instruction bit 3 |
| **4** | `RS` | Register Select | `0` = Instruction Register (Command), `1` = Data Register (RAM) |
| **5** | `RW` | Read / Write | `0` = Write to LCD, `1` = Read from LCD (write-only in typical setups) |
| **6** | `E` | Enable Pulse | Active clock. **Falling edge ($1 \to 0$) latches data** into the LCD |
| **7** | — | Unused | Available as general-purpose I/O on VIA Port B |

### 2.2 4-Bit Single-Port vs. 8-Bit Dual-Port (Port B/A) Modes

In hobbyist 6502 designs (such as Ben Eater's architecture), the HD44780 is often wired in an **8-bit dual-port configuration**:
- **VIA Port B (`$6000`):** Dedicated to the full 8-bit data bus (`DB0`–`DB7`).
- **VIA Port A (`$6001`):** Dedicated to control signals (`RS`, `R/W`, `E`).

While simple to code, an 8-bit configuration consumes 11 I/O pins across both VIA ports. 

**65c02-SIM optimizes this by implementing 4-bit multiplexed bus latching (`HD44780BusLatch`) entirely on Port B**:
- Consumes only 7 pins on Port B (`PB0`–`PB6`).
- Completely frees **Port A (`$6001`)** and its hardware handshake lines (`CA1`/`CA2`) for high-speed serial I/O, user peripherals, or SPI controllers.
- Legacy software initializing in 8-bit mode sends the standard 3-step synchronization sequence (`0x03`, `0x03`, `0x03`, followed by `0x02`), cleanly transitioning the simulated latch into 4-bit operation.

---

## 3. Bus Latch State Machine (`HD44780BusLatch`)

Because the 65c02 data bus is 8 bits wide and 4 data lines (`PB0`–`PB3`) are reserved for the LCD bus, the controller operates in **4-bit nibble mode**.

### 3.1 Falling-Edge Enable Detection

The HD44780 latches input data specifically on the **falling edge** of the Enable (`E`) signal. In [`HD44780BusLatch.h`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Hardware/Video/HD44780BusLatch.h):

```cpp
bool old_E = (old_val & 0x40) != 0; // PB6
bool new_E = (val & 0x40) != 0;     // PB6

if (old_E && !new_E) { // Detected falling edge
    Byte data_nibble = val & 0x0F;
    bool regSelect   = (val & 0x10) != 0; // PB4
    bool readWrite   = (val & 0x20) != 0; // PB5
    ...
}
```

### 3.2 State Transitions

```
                    +--------------------+
                    |  Uninitialized /   |
                    |    8-bit Mode      |
                    +--------------------+
                               |
              E-pulse with RS=0, Nibble=0x02
                               v
                    +--------------------+
                    |    4-bit Mode      |<-------------------+
                    | Waiting High Nibble|                    |
                    +--------------------+                    |
                               |                              |
                   E-pulse with High Nibble                   |
                               v                              |
                    +--------------------+                    |
                    |    4-bit Mode      |                    |
                    | Waiting Low Nibble |                    |
                    +--------------------+                    |
                               |                              |
                    E-pulse with Low Nibble                   |
                               v                              |
                     Assemble Full Byte:                      |
                 (High << 4) | Low Nibble --------------------+
```

1. **Power-On Reset:** Starts in 8-bit mode (`four_bit_mode = false`, `is_init = false`).
2. **Mode Switch (Command `0x02`):** When the CPU transmits command nibble `0x02` with `RS=0`, the latch switches into 4-bit mode (`four_bit_mode = true`, `is_init = true`).
3. **Two-Stage Assembly:**
   - **Step 1 (High Nibble):** The upper 4 bits are latched into `current_high_nibble` and `waiting_low_nibble` is set to `true`.
   - **Step 2 (Low Nibble):** The lower 4 bits are latched and combined with the upper bits:
     $$\text{Full Byte} = (\text{current\_high\_nibble} \ll 4) \mid \text{data\_nibble}$$
   - A `LatchResult` is emitted with `.ready = true`, passing the byte to `LCD::HandleCommand()` or `LCD::WriteCharToScreen()`.

---

## 4. HD44780 Command Reference

When `RS = 0`, byte transfers are decoded as control instructions:

| Opcode | Command Name | Bit Pattern (`D7`–`D0`) | Function / Description |
|:---:|:---|:---:|:---|
| `0x01` | **Clear Display** | `0000 0001` | Clears all 32 character cells to space (`' '`), resets `cursorX = 0`, `cursorY = 0`. |
| `0x02` | **Return Home** | `0000 001x` | Sets DDRAM address to 0. Returns cursor to $(0, 0)$ without altering display RAM. |
| `0x06` | **Entry Mode Set** | `0000 0110` | Configures cursor auto-increment mode (`cursor_increment = true`). |
| `0x0C` | **Display ON, Cursor OFF** | `0000 1100` | Enables display output, hides blinking/solid underline cursor. |
| `0x0E` | **Display ON, Cursor ON** | `0000 1110` | Enables display output and displays the cursor at the current position. |
| `0x28` | **Function Set** | `0010 1000` | Sets 4-bit bus mode (`DL=0`), 2-line display (`N=1`), and $5 \times 8$ character matrix (`F=0`). |
| `0x80` + $A$ | **Set DDRAM Address** | `1AAA AAAA` | Moves cursor directly to DDRAM address $A$ ($0 \dots 127$). |

### 4.1 DDRAM Memory Addressing

The HD44780 internal Display Data RAM (DDRAM) maps scan lines into distinct address spaces:

- **Line 1 (Row 0):** Addresses `$00` – `$0F` (Command: `$80` | `$00` through `$8F`)
- **Line 2 (Row 1):** Addresses `$40` – `$4F` (Command: `$80` | `$40` = `$C0` through `$CF`)

```cpp
// Address decoding in LCD::HandleCommand
if ((cmd & 0x80) == 0x80) {
    Byte addr = cmd & 0x7F;
    if (addr >= 0x40) {
        cursorY = 1;
        cursorX = addr - 0x40;
    } else {
        cursorY = 0;
        cursorX = addr;
    }
    cursorX = std::min(cursorX, 15);
}
```

### 4.2 DDRAM vs. CGRAM Character Generation

The HD44780 controller distinguishes between two distinct internal RAM spaces for character rendering:

| Characteristic | Display Data RAM (DDRAM) | Character Generator RAM (CGRAM) |
|:---|:---|:---|
| **Purpose** | Stores display character indices (which glyph to show) | Stores custom user-defined font bit patterns ($5 \times 8$ dots) |
| **Capacity** | 80 bytes ($2 \times 40$ characters) | 64 bytes (8 user-programmable characters, 8 bytes each) |
| **Addressing Command** | `0x80 | DDRAM_ADDR` (`D7 = 1`) | `0x40 | CGRAM_ADDR` (`D6 = 1`, `D7 = 0`) |
| **Visible Addresses (16×2)** | Line 1: `0x00`–`0x0F`; Line 2: `0x40`–`0x4F` | Character Codes `0x00`–`0x07` (mirrored at `0x08`–`0x0F`) |
| **Data Format** | Standard 8-bit ASCII character codes | 5 active pixel bits per byte ($0 \dots 7$ rows per glyph) |

#### Custom Character Generation Workflow:
1. Issue command `0x40 | (char_index << 3)` to select the base CGRAM row address.
2. Write 8 consecutive bytes with `RS = 1`, where each byte defines 5 horizontal pixels (`bits [4:0]`).
3. Return to DDRAM mode by issuing command `0x80 | ddram_addr`.
4. Transmit character code `$00`–`$07` with `RS = 1` to render the custom glyph on display.

#### Emulator Pipeline Status:
In 65c02-SIM, the virtual LCD focuses on standard ASCII rendering through its $16 \times 2$ character buffer (`std::array<std::array<char, 16>, 2> screen`) and host terminal mirroring (`onChar`). DDRAM address instructions (`0x80 | addr`) position `cursorX` and `cursorY` accurately across both lines, allowing existing LCD menu routines, formatted prints, and cursor addressing to function identically to physical hardware.

---

## 5. Character Writing & Terminal Redirection

When `RS = 1`, the assembled byte represents an ASCII character code:

1. **Screen Buffer Write:** Characters are written to `screen[cursorY][cursorX]`.
2. **Auto-Increment / Wrapping:**
   - With `cursor_increment = true`, `cursorX` increments after each character.
   - If `cursorX` reaches 16, it automatically wraps to line 2 (`cursorX = 0`, `cursorY = 1`).
   - If line 2 overflows, the cursor wraps back to the start of line 1 (`cursorX = 0`, `cursorY = 0`).
3. **Newline Handling:** A newline character (`\n` or `\r`) resets `cursorX = 0` and advances `cursorY`.
4. **Terminal Callback (`onChar`):** When attached, characters written to the LCD are mirrored to the interactive terminal/console, translating `\n` to `\r\n`.

---

## 6. Physical vs. Simulated Timing Constraints

| Operation | Physical HD44780 Hardware | 65c02-SIM Virtual Hardware |
|:---|:---:|:---:|
| Clear Display (`0x01`) | $1.52\text{ ms}$ delay required | **Instantaneous** |
| Return Home (`0x02`) | $1.52\text{ ms}$ delay required | **Instantaneous** |
| Standard Command / Write | $37\text{ \mu s}$ execution time | **Instantaneous** |
| Enable Pulse Width ($t_{PW}$) | Minimum $450\text{ ns}$ | **Single clock edge evaluation** |
| Busy Flag Polling (`BF`) | Read bit 7 with `RS=0, RW=1` | Not required; latch is always ready |

In physical hardware, 6502 code must execute software delay loops or poll the Busy Flag before sending consecutive commands. In 65c02-SIM, commands execute instantaneously upon the falling edge of `E`, ensuring existing code with delay loops runs perfectly while allowing optimized emulator routines to run at maximum bus throughput.

---

## 7. Programming Examples

### 7.1 65c02 Assembly: Complete LCD Driver

```assembly
; =====================================================================
; LCD Driver for 65c02 / MOS 6522 VIA
; Interfaced via Port B ($6000)
; =====================================================================
PORTB   = $6000
DDRB    = $6002

; Control signal bitmasks on PORTB
E_BIT   = %01000000     ; PB6
RS_BIT  = %00010000     ; PB4

; ---------------------------------------------------------------------
; LCD_Init: Initializes LCD into 4-bit mode, 2 lines, 5x8 font
; ---------------------------------------------------------------------
LCD_Init:
    LDA #$FF
    STA DDRB            ; Configure all Port B pins as outputs

    ; 1. Three wakeup attempts with 8-bit mode command (0x03)
    LDA #$03
    JSR LCD_PulseE
    LDA #$03
    JSR LCD_PulseE
    LDA #$03
    JSR LCD_PulseE

    ; 2. Switch to 4-bit interface mode (0x02)
    LDA #$02
    JSR LCD_PulseE

    ; 3. Function Set: 4-bit, 2 lines, 5x8 font ($28)
    LDA #$28
    JSR LCD_SendCmd

    ; 4. Display ON, Cursor OFF ($0C)
    LDA #$0C
    JSR LCD_SendCmd

    ; 5. Entry Mode Set: Increment cursor, no shift ($06)
    LDA #$06
    JSR LCD_SendCmd

    ; 6. Clear display ($01)
    LDA #$01
    JSR LCD_SendCmd
    RTS

; ---------------------------------------------------------------------
; LCD_SendCmd: Sends command byte in A (RS = 0)
; ---------------------------------------------------------------------
LCD_SendCmd:
    PHA
    LSR
    LSR
    LSR
    LSR                 ; Extract high nibble
    JSR LCD_PulseE

    PLA                 ; Restore low nibble
    AND #$0F
    JSR LCD_PulseE
    RTS

; ---------------------------------------------------------------------
; LCD_SendChar: Sends ASCII character in A (RS = 1)
; ---------------------------------------------------------------------
LCD_SendChar:
    PHA
    LSR
    LSR
    LSR
    LSR
    ORA #RS_BIT         ; Set RS=1
    JSR LCD_PulseE

    PLA
    AND #$0F
    ORA #RS_BIT         ; Set RS=1
    JSR LCD_PulseE
    RTS

; ---------------------------------------------------------------------
; LCD_PulseE: Emits a high-to-low pulse on Enable (PB6)
; Input: A = Nibble + Control bits (RS) with E=0
; ---------------------------------------------------------------------
LCD_PulseE:
    STA PORTB           ; E = 0
    ORA #E_BIT
    STA PORTB           ; E = 1
    AND #~E_BIT
    STA PORTB           ; E = 0 (Falling edge latches data!)
    RTS

; ---------------------------------------------------------------------
; LCD_PrintString: Prints null-terminated string pointed by ($00,$01)
; ---------------------------------------------------------------------
LCD_PrintString:
    LDY #0
@Loop:
    LDA ($00),Y
    BEQ @Done
    JSR LCD_SendChar
    INY
    BNE @Loop
@Done:
    RTS
```

### 7.2 C Language Example (Direct VIA Access)

```c
#include <stdint.h>

#define VIA_PORTB (*(volatile uint8_t*)0x6000)
#define VIA_DDRB  (*(volatile uint8_t*)0x6002)

#define LCD_RS 0x10  // PB4
#define LCD_E  0x40  // PB6

static void lcd_pulse_nibble(uint8_t val) {
    VIA_PORTB = val;               // E = 0
    VIA_PORTB = val | LCD_E;       // E = 1
    VIA_PORTB = val & (uint8_t)~LCD_E; // E = 0 (Falling edge)
}

void lcd_command(uint8_t cmd) {
    lcd_pulse_nibble((cmd >> 4) & 0x0F);
    lcd_pulse_nibble(cmd & 0x0F);
}

void lcd_putc(char c) {
    lcd_pulse_nibble(((c >> 4) & 0x0F) | LCD_RS);
    lcd_pulse_nibble((c & 0x0F) | LCD_RS);
}

void lcd_set_cursor(uint8_t col, uint8_t row) {
    uint8_t addr = (row == 0) ? col : (0x40 + col);
    lcd_command(0x80 | (addr & 0x7F));
}

void lcd_init(void) {
    VIA_DDRB = 0xFF;  // Configure all pins as output

    // Wakeup sequence
    lcd_pulse_nibble(0x03);
    lcd_pulse_nibble(0x03);
    lcd_pulse_nibble(0x03);

    // Switch to 4-bit mode
    lcd_pulse_nibble(0x02);

    lcd_command(0x28); // 4-bit mode, 2 lines, 5x8
    lcd_command(0x0C); // Display ON, Cursor OFF
    lcd_command(0x06); // Entry mode auto-increment
    lcd_command(0x01); // Clear display
}

void lcd_puts(const char* str) {
    while (*str) {
        lcd_putc(*str++);
    }
}
```
