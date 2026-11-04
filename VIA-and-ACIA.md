# VIA (MOS 6522) & ACIA (MOS 6551) Hardware Reference

**Source Files:**
- VIA Core: [`src/Hardware/Comm/VIA.h`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Hardware/Comm/VIA.h) / [`src/Hardware/Comm/VIA.cpp`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Hardware/Comm/VIA.cpp)
- VIA Timers: [`src/Hardware/Comm/VIATimer1.h`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Hardware/Comm/VIATimer1.h), [`src/Hardware/Comm/VIATimer2.h`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Hardware/Comm/VIATimer2.h)
- VIA Shift Register: [`src/Hardware/Comm/VIAShiftRegister.h`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Hardware/Comm/VIAShiftRegister.h)
- ACIA Core: [`src/Hardware/Comm/ACIA.h`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Hardware/Comm/ACIA.h) / [`src/Hardware/Comm/ACIA.cpp`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Hardware/Comm/ACIA.cpp)
- ACIA Framing & Timing: [`src/Hardware/Comm/ACIAFrameConfig.h`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Hardware/Comm/ACIAFrameConfig.h)
- Test Suites: [`src/UnitTests/VIA_Test.cpp`](file:///home/manu/Documents/GitHub/65c02-SIM/src/UnitTests/VIA_Test.cpp), [`src/UnitTests/ACIA_Test.cpp`](file:///home/manu/Documents/GitHub/65c02-SIM/src/UnitTests/ACIA_Test.cpp)

---

## 1. Overview & System Memory Maps

The 65c02-SIM platform emulates two quintessential communications and I/O interface chips of the 6502 architecture:
1. **MOS 6522 Versatile Interface Adapter (VIA):** Parallel I/O, dual 16-bit timers, hardware handshake lines, and an 8-bit bidirectional shift register.
2. **MOS 6551 Asynchronous Communications Interface Adapter (ACIA):** UART serial controller featuring a programmable baud rate generator, full modem control lines, and configurable data framing.

### 1.1 Address Space Allocations

The emulator supports both the classic breadboard system memory map and the compact high-memory microDOS / Cartridge MMIO map:

| Peripheral | Default System Layout | microDOS / Cartridge Layout | Size | Primary Function |
|:---|:---:|:---:|:---:|:---|
| **ACIA 6551** | `0x5000` – `0x5003` | `0x7FC0` – `0x7FC3` | 4 Bytes | Serial Console / Terminal I/O |
| **VIA 6522** | `0x6000` – `0x600F` | `0x7FD0` – `0x7FDF` | 16 Bytes | LCD Driver, Hardware Timers, Parallel Bus |

---

## 2. MOS 6522 VIA (Versatile Interface Adapter)

### 2.1 Complete Register Reference

| Offset | Name | Type | Description |
|:---:|:---|:---:|:---|
| **`$0`** | `ORB` / `IRB` | R/W | **Output/Input Register B:** Reflects pin states or latched inputs; controls LCD. |
| **`$1`** | `ORA` / `IRA` | R/W | **Output/Input Register A:** With handshaking on CA1/CA2 control lines. |
| **`$2`** | `DDRB` | R/W | **Data Direction Register B:** `1` = Output pin, `0` = Input pin. |
| **`$3`** | `DDRA` | R/W | **Data Direction Register A:** `1` = Output pin, `0` = Input pin. |
| **`$4`** | `T1C_L` | R/W | **Timer 1 Counter Low:** Reads low byte of counter; writes low byte of latch. |
| **`$5`** | `T1C_H` | R/W | **Timer 1 Counter High:** Reads high byte of counter; write initiates countdown! |
| **`$6`** | `T1L_L` | R/W | **Timer 1 Latch Low:** Accesses low byte of reload latch without affecting counter. |
| **`$7`** | `T1L_H` | R/W | **Timer 1 Latch High:** Accesses high byte of reload latch. |
| **`$8`** | `T2C_L` | R/W | **Timer 2 Counter Low:** Reads low byte of counter; writes low byte of latch. |
| **`$9`** | `T2C_H` | R/W | **Timer 2 Counter High:** Reads high byte of counter; write initiates countdown! |
| **`$A`** | `SR` | R/W | **Shift Register:** 8-bit serial input/output register. |
| **`$B`** | `ACR` | R/W | **Auxiliary Control Register:** Configures timers, shift register, and latching. |
| **`$C`** | `PCR` | R/W | **Peripheral Control Register:** Configures CA1, CA2, CB1, CB2 control lines. |
| **`$D`** | `IFR` | R/W | **Interrupt Flag Register:** Set flags for active pending interrupts. |
| **`$E`** | `IER` | R/W | **Interrupt Enable Register:** Masks and enables individual IRQ sources. |
| **`$F`** | `ORA_NH` | R/W | **Output Register A (No Handshake):** Reads/writes Port A without affecting CA lines. |

---

### 2.2 Auxiliary Control Register (`ACR`)

Controls internal operating modes of Timer 1, Timer 2, the Shift Register, and Port Input Latching:

```
Bit:    7       6       5       4       3       2       1       0
      +-------+-------+-------+-------+-------+-------+-------+-------+
      |  T1-PB7  |  T1-MODE  |  T2-MODE  |   SHIFT REGISTER   |  PA-L |  PB-L |
      +-------+-------+-------+-------+-------+-------+-------+-------+
```

| Bits | Field | Settings / Hardware Operation |
|:---:|:---|:---|
| **7** | `T1-PB7` | **Timer 1 Output on PB7:** `0` = Disabled (PB7 is normal I/O), `1` = Enable square wave / pulse output on PB7. |
| **6** | `T1-MODE` | **Timer 1 Operating Mode:** `0` = One-Shot mode (underflows, interrupts once, continues counting); `1` = Continuous Free-Running mode (automatically reloads from latches, generates periodic IRQs). |
| **5** | `T2-MODE` | **Timer 2 Operating Mode:** `0` = Timed interrupt (counts system clock cycles $\Phi_2$); `1` = Pulse counting mode (decrements on negative transitions of external pin **PB6**). |
| **4:2** | `SR-MODE` | **Shift Register Operating Mode:**<br>`000` = Disabled<br>`001` = Shift In under control of Timer 2<br>`010` = Shift In under control of system clock $\Phi_2$<br>`011` = Shift In under control of external clock pulses on **CB1**<br>`100` = Shift Out free-running at Timer 2 rate<br>`101` = Shift Out under control of Timer 2<br>`110` = Shift Out under control of system clock $\Phi_2$<br>`111` = Shift Out under control of external clock pulses on **CB1** |
| **1** | `PB-L` | **Port B Input Latching:** `0` = Disabled, `1` = Latch input pins on CB1 transition. |
| **0** | `PA-L` | **Port A Input Latching:** `0` = Disabled, `1` = Latch input pins on CA1 transition. |

---

### 2.3 Peripheral Control Register (`PCR`)

Configures edge-detection sensitivity and automatic handshake modes for the 4 external control lines:

```
Bit:    7       6       5       4       3       2       1       0
      +-------+-------+-------+-------+-------+-------+-------+-------+
      |      CB2 CONTROL      |  CB1  |      CA2 CONTROL      |  CA1  |
      +-------+-------+-------+-------+-------+-------+-------+-------+
```

| Field | Bits | Mode Configuration |
|:---|:---:|:---|
| **CA1 Control** | `[0]` | `0` = Negative active edge (high-to-low transition sets IFR bit 1).<br>`1` = Positive active edge (low-to-high transition sets IFR bit 1). |
| **CA2 Control** | `[3:1]` | `000` = Input mode: negative active edge.<br>`001` = Independent interrupt input: negative edge.<br>`010` = Input mode: positive active edge.<br>`011` = Independent interrupt input: positive edge.<br>`100` = Handshake output: pulses low after reading/writing Port A.<br>`101` = Pulse output: 1-cycle low pulse on write.<br>`110` = Manual output: driven LOW.<br>`111` = Manual output: driven HIGH. |
| **CB1 Control** | `[4]` | `0` = Negative active edge.<br>`1` = Positive active edge. |
| **CB2 Control** | `[7:5]` | Identical modes to CA2, controlling line CB2 and Port B operations. |

---

### 2.4 Interrupt Flag (`IFR`) & Interrupt Enable (`IER`) Registers

```
Bit:    7       6       5       4       3       2       1       0
      +-------+-------+-------+-------+-------+-------+-------+-------+
      |  IRQ  |  T1   |  T2   |  CB1  |  CB2  |  SR   |  CA1  |  CA2  |
      +-------+-------+-------+-------+-------+-------+-------+-------+
```

- **Bit 7 (`IRQ`):** In `IFR`, asserts `1` if any interrupt flag is active AND enabled in `IER`:
  $$\text{IFR}[7] = (\text{IFR}[6:0] \ \& \ \text{IER}[6:0]) \ne 0$$
- **Writing to `IER`:** Bit 7 controls setting vs. clearing:
  - If Bit 7 is `1`: bits with `1` are **enabled** (`ier |= (data & 0x7F)`).
  - If Bit 7 is `0`: bits with `1` are **disabled** (`ier &= ~(data & 0x7F)`).
- **Clearing `IFR` Flags:** Writing a byte with `1` in bit positions clears those specific flags. Reading/writing associated data registers (e.g. reading Port A clears CA1/CA2) automatically clears corresponding flags.

---

### 2.5 Hardware Timers & Shift Register Implementation

1. **Timer 1 (`VIATimer1`):** A 16-bit down-counter with automatic latch reload. In continuous mode (`ACR[6] = 1`), underflowing from `0x0000` to `0xFFFF` reloads `t1l` on the next clock cycle without CPU intervention. If `ACR[7] = 1`, line PB7 toggles on every underflow, generating high-precision hardware square waves.
2. **Timer 2 (`VIATimer2`):** In timed mode, acts as a one-shot countdown timer. In pulse-counting mode (`ACR[5] = 1`), clock ticks are ignored and the counter decrements only when `via.SetInputB()` detects a falling edge on bit 6 (`PB6`).
3. **Shift Register (`VIAShiftRegister`):** Shifts 8 bits on pin **CB2**, clocked internally by Timer 2 or $\Phi_2$, or externally via clock pulses on **CB1**. When all 8 bits have shifted, bit 2 of `IFR` is asserted.

---

## 3. MOS 6551 ACIA (Asynchronous Communications Interface)

The 6551 ACIA provides full-duplex asynchronous serial communication, bridging the 65c02 system bus with host serial terminals and GUI consoles.

### 3.1 ACIA Register Map

| Offset | Name | Access | Hardware Function |
|:---:|:---|:---:|:---|
| **`$0`** | `ACIA_DATA` | R/W | **Read:** Transmit Data Register (`rxData`), clears RDRF flag.<br>**Write:** Transmit Data Register (`txData`), clears TDRE flag. |
| **`$1`** | `ACIA_STATUS` | R | **Status Register:** Reports receiver, transmitter, error, and modem states. |
| **`$2`** | `ACIA_CMD` | R/W | **Command Register:** Parity controls, transmitter interrupt control, receiver IRQ. |
| **`$3`** | `ACIA_CTRL` | R/W | **Control Register:** Baud rate selection, word length, stop bits. |

---

### 3.2 Status Register (`ACIA_STATUS`)

```
Bit:    7       6       5       4       3       2       1       0
      +-------+-------+-------+-------+-------+-------+-------+-------+
      |  IRQ  |  DSR  |  DCD  | TDRE  | RDRF  | OVRN  |  FE   |  PE   |
      +-------+-------+-------+-------+-------+-------+-------+-------+
```

| Bit | Mask | Flag | Definition |
|:---:|:---:|:---|:---|
| **7** | `0x80` | `IRQ` | **Interrupt Request:** `1` = ACIA has an active, unserviced interrupt pending. |
| **6** | `0x40` | `DSR` | **Data Set Ready:** State of incoming `/DSR` modem handshake line (`0` = Ready). |
| **5** | `0x20` | `DCD` | **Data Carrier Detect:** State of `/DCD` modem carrier line (`0` = Detected). |
| **4** | `0x10` | `TDRE` | **Transmitter Data Register Empty:** `1` = Ready to accept new byte for transmission. |
| **3** | `0x08` | `RDRF` | **Receiver Data Register Full:** `1` = Received byte waiting in `ACIA_DATA`. Cleared on read. |
| **2** | `0x04` | `OVRN` | **Overrun Error:** `1` = New byte received before previous byte was read. |
| **1** | `0x02` | `FE` | **Framing Error:** `1` = Stop bit was not received at expected time. |
| **0** | `0x01` | `PE` | **Parity Error:** `1` = Received parity bit did not match configured parity. |

---

### 3.3 Command Register (`ACIA_CMD`)

```
Bit:    7       6       5       4       3       2       1       0
      +-------+-------+-------+-------+-------+-------+-------+-------+
      |  PARITY CONTROL       | ECHO  |  TX IRQ CTRL  | RX_DIS|  DTR  |
      +-------+-------+-------+-------+-------+-------+-------+-------+
```

| Field | Bits | Description |
|:---|:---:|:---|
| **Parity Controls** | `[7:5]` | `xx0` = Parity disabled; `001` = Odd parity; `011` = Even parity; `101` = Mark parity; `111` = Space parity. |
| **Echo Mode** | `[4]` | `0` = Normal operation; `1` = Echo mode (received characters automatically echoed to transmitter). |
| **Tx IRQ Control** | `[3:2]` | `00` = `/RTS` High, Tx IRQ disabled;<br>`01` = `/RTS` Low, **Tx IRQ enabled** when TDRE=1;<br>`10` = `/RTS` Low, Tx IRQ disabled;<br>`11` = `/RTS` Low, Transmit Break on Tx line. |
| **Rx IRQ Disable** | `[1]` | `0` = **Rx IRQ enabled** (triggers IRQ when RDRF=1); `1` = Rx IRQ disabled. |
| **DTR Control** | `[0]` | `0` = `/DTR` line High (terminal not ready); `1` = `/DTR` line Low (terminal ready). |

---

### 3.4 Control Register (`ACIA_CTRL`) & Baud Rates

```
Bit:    7       6       5       4       3       2       1       0
      +-------+-------+-------+-------+-------+-------+-------+-------+
      | S-BIT | WORD LENGTH   | R-CLK |       BAUD RATE SELECT        |
      +-------+-------+-------+-------+-------+-------+-------+-------+
```

- **Stop Bits (Bit 7):** `0` = 1 Stop Bit; `1` = 2 Stop Bits (or $1\frac{1}{2}$ with 5-bit words).
- **Word Length (Bits [6:5]):** `00` = 8 Bits, `01` = 7 Bits, `10` = 6 Bits, `11` = 5 Bits.
- **Receiver Clock (Bit 4):** `0` = External receiver clock; `1` = Internal baud rate generator.
- **Baud Rate Generator (Bits [3:0]):**

| Value | Baud Rate | Value | Baud Rate | Value | Baud Rate | Value | Baud Rate |
|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|
| `0000` | 16x External | `0100` | 134.5 Baud | `1000` | 1,200 Baud | `1100` | 4,800 Baud |
| `0001` | 50 Baud | `0101` | 150 Baud | `1001` | 1,800 Baud | `1101` | 7,200 Baud |
| `0010` | 75 Baud | `0110` | 300 Baud | `1010` | 2,400 Baud | `1110` | **9,600 Baud** |
| `0011` | 110 Baud | `0111` | 600 Baud | `1011` | 3,600 Baud | `1111` | **19,200 Baud** |

### 3.5 Cycle-Accurate Shifting & UART Framing

The emulator's [`ACIAFrameConfig`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Hardware/Comm/ACIAFrameConfig.h) calculates the exact serial frame bit count:

$$\text{Total Frame Bits} = 1\text{ (Start Bit)} + \text{WordLength} + \text{ParityBit} + \text{StopBits}$$

When a byte is written to `ACIA_DATA`, the ACIA calculates transmission delay based on the simulated clock rate:

$$\text{shiftRegisterTicks} = \frac{\text{Instructions Per Second (IPS)} \times \text{TotalFrameBits}}{\text{BaudRate}}$$

The byte is held in the internal shift register until the delay expires, at which point `outputCallback` delivers the character to the GUI **Console Window**, accurately matching real UART transmission speed!

### 3.6 Host Serial Bridge Subsystem (`SerialIOBridge`)

Asynchronous communication between host I/O (GUI text console, standard input, or scripted automation) and the ACIA core is managed by [`SerialIOBridge`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Hardware/Core/Emulator/SerialIOBridge.h):

```
+--------------------+      InjectKey()       +-------------------+
| Host GUI / Console | ─────────────────────> |  Thread-Safe FIFO |
| Keyboard Input     |      InjectText()      |    inputBuffer    |
+--------------------+                        +---------+---------+
                                                        │
                                                        │ ProcessSerialInput()
                                                        │ (Paced by baudDelay)
                                                        v
                                              +---------+---------+
                                              |    ACIA 6551      |
                                              | ReceiveData(chr)  |
                                              +---------+---------+
                                                        │
                         ForwardOutputIfBridged()       │ ACIA Transmit
                      (when aciaEspBridge is true)      v
                                              +-------------------+
                                              |   ESP8266 MMIO    |
                                              |   (0x5004 DATA)   |
                                              +-------------------+
```

#### Key Architecture Responsibilities:
1. **Thread-Safe Input Queue:** Keystrokes injected from host threads via `InjectKey(char)` or pasted text via `InjectText(string_view)` are stored in an internal `std::deque<char>` protected by `bufferMutex`. Line ending conversion (`\n` $\to$ `\r`) is automatically performed for terminal compatibility.
2. **Baud Rate Delay Pacing:** Characters are not dumped into the CPU instantly. Every CPU cycle, `DecrementBaudDelay()` ticks down. Once `baudDelay <= 0` and `acia.CanReceive()` confirms the receiver is empty (`RDRF = 0`), the next character is delivered to `acia.ReceiveData(chr)`, and `baudDelay` is recharged:
   $$\text{baudDelay} = \frac{\text{targetIps} \times 10}{\text{baudRate}}$$
   This prevents keyboard buffer inundation and ensures polled software routines don't suffer from receiver overrun errors (`OVRN = 1`).
3. **Interrupt Dispatch:** If receiver interrupts are enabled in `ACIA_CMD`, delivering a character triggers `acia.HasIRQ()` and atomically flags `pendingInterruptAny = true` to alert the CPU core.
4. **ACIA-to-ESP8266 Modem Bridging (`ForwardOutputIfBridged`):**
   When `aciaEspBridge` is enabled (via `SetACIAESPBridge(true)`), bytes transmitted over the ACIA serial line are automatically piped into the virtual ESP8266:
   ```cpp
   void SerialIOBridge::ForwardOutputIfBridged(char c, Hardware::ESP8266& esp8266) {
       if (!aciaEspBridge) return;
       esp8266.RecordTraffic(TrafficDirection::TX, TrafficChannel::ATCommand, -1, "ACIA -> ESP8266", std::string(1, c));
       esp8266.Write(0x5004, static_cast<uint8_t>(c));
   }
   ```
   This allows vintage 6502 applications written for standalone serial modems to communicate directly with the virtual Wi-Fi SoC and issue AT commands over UART without requiring microDOS MMIO drivers.

---

## 4. Programming Examples

### 4.1 65c02 Assembly: Polled Serial Console Output & Input

```assembly
; =====================================================================
; ACIA 6551 Serial Console Driver
; MMIO base address: $5000 (Default) or $7FC0 (microDOS)
; =====================================================================
ACIA_DATA   = $5000
ACIA_STATUS = $5001
ACIA_CMD    = $5002
ACIA_CTRL   = $5003

ACIA_Init:
    ; Soft reset
    STA ACIA_STATUS
    ; 8-N-1, 19200 Baud ($1F)
    LDA #%00011111
    STA ACIA_CTRL
    ; Parity off, Echo off, Rx IRQ disabled, DTR active ($0B)
    LDA #%00001011
    STA ACIA_CMD
    RTS

; ---------------------------------------------------------------------
; ACIA_PutChar: Sends byte in Accumulator
; ---------------------------------------------------------------------
ACIA_PutChar:
    PHA
@WaitTx:
    LDA ACIA_STATUS
    AND #$10            ; Check TDRE (bit 4)
    BEQ @WaitTx         ; Loop until transmitter is empty
    PLA
    STA ACIA_DATA       ; Transmit character
    RTS

; ---------------------------------------------------------------------
; ACIA_GetChar: Blocks until a character is received, returns in A
; ---------------------------------------------------------------------
ACIA_GetChar:
@WaitRx:
    LDA ACIA_STATUS
    AND #$08            ; Check RDRF (bit 3)
    BEQ @WaitRx         ; Loop until receiver has data
    LDA ACIA_DATA       ; Read byte (clears RDRF)
    RTS
```

### 4.2 65c02 Assembly: Continuous Timer 1 Interrupt on VIA

```assembly
; =====================================================================
; VIA 6522 Periodic 1 kHz Timer Interrupt
; Base address: $6000
; =====================================================================
VIA_BASE    = $6000
T1C_L       = VIA_BASE + $04
T1C_H       = VIA_BASE + $05
T1L_L       = VIA_BASE + $06
T1L_H       = VIA_BASE + $07
ACR         = VIA_BASE + $0B
IER         = VIA_BASE + $0E

VIA_InitTimer:
    ; 1. Set Timer 1 to continuous reload mode (ACR[6] = 1)
    LDA #%01000000
    STA ACR

    ; 2. Set reload period for 1 ms at 1 MHz clock (1000 cycles = $03E8)
    LDA #<$03E8
    STA T1L_L
    LDA #>$03E8
    STA T1C_H           ; Writing high counter starts timer and loads latches!

    ; 3. Enable Timer 1 Interrupt in IER (Bit 7=1 to set, Bit 6=Timer 1)
    LDA #%11000000
    STA IER
    CLI                 ; Enable CPU interrupts
    RTS
```
