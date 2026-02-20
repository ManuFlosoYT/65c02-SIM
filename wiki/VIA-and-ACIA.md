# VIA and ACIA — I/O Chips

## VIA — MOS 6522 Versatile Interface Adapter

**File:** `Hardware/VIA.h` / `Hardware/VIA.cpp`  
**Namespace:** `Hardware::VIA`  
**Base address:** `0x6000`

### Overview

The **VIA 6522** is the main I/O interface chip of the system. It provides bidirectional parallel I/O ports, two 16-bit timers, and a shift register. In Ben Eater's design it is used primarily to control the **LCD display**.

### Registers

| Address | Name | Description |
|---------|------|-------------|
| `0x6000` | `PORTB` | Port B data register |
| `0x6001` | `PORTA` | Port A data register |
| `0x6002` | `DDRB` | Data Direction Register B (1=output, 0=input) |
| `0x6003` | `DDRA` | Data Direction Register A |
| `0x6004` | `T1C_L` | Timer 1 Counter — low byte |
| `0x6005` | `T1C_H` | Timer 1 Counter — high byte |
| `0x6006` | `T1L_L` | Timer 1 Latch — low byte |
| `0x6007` | `T1L_H` | Timer 1 Latch — high byte |
| `0x6008` | `T2C_L` | Timer 2 Counter — low byte |
| `0x6009` | `T2C_H` | Timer 2 Counter — high byte |
| `0x600A` | `SR` | Shift Register |
| `0x600B` | `ACR` | Auxiliary Control Register |
| `0x600C` | `PCR` | Peripheral Control Register |
| `0x600D` | `IFR` | Interrupt Flag Register |
| `0x600E` | `IER` | Interrupt Enable Register |
| `0x600F` | `ORA_NH` | Output Register A (no handshake) |

### Timers

**Timer 1 (T1)** — 16 bits, two modes:
- **One-shot:** generates an IRQ when it expires and keeps counting.
- **Continuous (ACR bit 6=1):** reloads from the latch and generates periodic IRQs.
- Optionally controls the state of **PB7** (ACR bit 7).

**Timer 2 (T2)** — 16 bits, two modes:
- **One-shot:** generates an IRQ when it expires and stops.
- **Pulse counting (ACR bit 5=1):** counts edges on the PB6 input.

### Shift Register (SR)

The SR transfers 8 bits serially. The mode is selected with `ACR[4:2]`:

| Value | Mode |
|-------|------|
| `000` | Disabled |
| `010` | Shift In (PHI2) |
| `011` | Shift In (external CB1) |
| `100` | Shift Out (free run / T2) |
| `101` | Shift Out (T2) |
| `110` | Shift Out (PHI2) |
| `111` | Shift Out (external CB1) |

### Interrupts

The VIA generates IRQs controlled by `IER` and `IFR`. When an interrupt condition occurs, the corresponding bit in `IFR` is set. If that bit is enabled in `IER`, the CPU's IRQ line is asserted.

### Clock

On each emulator cycle, `via.Clock()` is called to decrement the timers and update the shift register according to the active mode.

### Port B callback

The VIA notifies Port B changes via a callback:

```cpp
via.SetPortBCallback([](Byte val) {
    lcd.Update(val);    // Update the LCD with the new value
});
```

---

## ACIA — MOS 6551 Asynchronous Communications Interface

**File:** `Hardware/ACIA.h` / `Hardware/ACIA.cpp`  
**Namespace:** `Hardware::ACIA`  
**Base address:** `0x5000`

### Overview

The **ACIA 6551** provides an asynchronous serial communications interface (UART). In the emulator it is used for the **text console**: the CPU writes characters to the data register and the emulator forwards them to the GUI console.

### Registers

| Address | Name | Description |
|---------|------|-------------|
| `0x5000` | `ACIA_DATA` | Data — write: send character; read: receive character |
| `0x5001` | `ACIA_STATUS` | Status — bit 7: received data available |
| `0x5002` | `ACIA_CMD` | Command — interface configuration |
| `0x5003` | `ACIA_CTRL` | Control — baud rate, data bits, stop bits |

### Operation

**Sending characters (CPU → console):**
1. The CPU writes a byte to `0x5000`.
2. The write hook invokes the `outputCallback`.
3. The GUI displays the character in the **Console Window**.

**Receiving characters (keyboard → CPU):**
1. The GUI calls `emulator.InjectKey(c)`.
2. The byte is added to the emulator's `inputBuffer`.
3. The emulator places the byte at `0x5000` and sets bit 7 of `0x5001`.
4. If the CPU was in `WAI` mode, it wakes up and reads the data.

### Output callback

```cpp
acia.SetOutputCallback([](char c) {
    console.Append(c);    // Append character to the console
});
```

### Baud rate delay

The emulator implements a `baudDelay` proportional to the transmission speed configured in `ACIA_CTRL`, to faithfully simulate the per-byte transmission time.
