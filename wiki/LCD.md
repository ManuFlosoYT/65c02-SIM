# LCD — 2×16 Character LCD Display

**File:** `Hardware/LCD.h` / `Hardware/LCD.cpp`  
**Namespace:** `Hardware::LCD`

## Overview

The emulator emulates a **2-row × 16-column LCD display** compatible with the **HD44780** controller, as used by Ben Eater in his 65c02 project. The LCD is controlled through Port B of the VIA 6522.

## Pin mapping on VIA Port B

VIA Port B connects to the LCD as follows (Ben Eater's schematic):

| PB bit | LCD signal | Description |
|--------|------------|-------------|
| 0 | DB4 | Data bit 4 |
| 1 | DB5 | Data bit 5 |
| 2 | DB6 | Data bit 6 |
| 3 | DB7 | Data bit 7 |
| 4 | RS | Register Select: 0=command, 1=data |
| 5 | RW | Read/Write: 0=write (always 0 in this design) |
| 6 | E | Enable (falling edge triggers the transfer) |

## 4-bit mode

The LCD operates in **4-bit nibble mode**. Each byte is sent in two transfers:
1. **High nibble** (bits 7–4)
2. **Low nibble** (bits 3–0)

The falling edge of the `E` signal (bit 6 of Port B) indicates when the data is ready.

## Initialization

The LCD initialization sequence is detected automatically. Until it completes, `IsInitialized()` returns `false` and no characters are processed.

## Supported commands

When `RS=0`, the byte sent is a **command**:

| Command | Description |
|---------|-------------|
| `0x01` | Clear display — clears the screen and places cursor at (0,0) |
| `0x02` | Return home — moves cursor to (0,0) without clearing |
| `0x28` | Function Set — 4-bit mode, 2 lines, 5×8 font |
| `0x0C` | Display ON, cursor OFF |
| `0x0E` | Display ON, cursor ON |
| `0x06` | Increment cursor, no scroll |
| `0x80` | Set DDRAM address — cursor on line 1 |
| `0xC0` | Set DDRAM address — cursor on line 2 |

## Character writing

When `RS=1`, the byte is interpreted as an **ASCII character** and written at the current cursor position. The cursor advances automatically. At the end of line 1, it continues on line 2.

## Emulator API

```cpp
// Update LCD with the VIA Port B value
lcd.Update(portBVal);

// Read the current screen state (2×16 array)
const char (&screen)[2][16] = lcd.GetScreen();

// Check whether the LCD has been initialized
bool ready = lcd.IsInitialized();

// Receive each printed character (for debugging)
lcd.SetOutputCallback([](char c) {
    std::cout << c;
});
```

## Frontend display

The **LCD Window** in the GUI shows the contents of the `screen[2][16]` array in real time, simulating the appearance of a physical LCD screen.
