# Hardware — Main emulator orchestrator and memory system

## `Emulator` class

`Core::Emulator` is the **central orchestrator** of the simulator. It coordinates all hardware components, manages the emulation thread, and exposes a clean API for the graphical interface.

**File:** `Hardware/Core/Emulator.h` / `Hardware/Core/Emulator.cpp`

### Instantiation and lifecycle

```cpp
Core::Emulator emulator;

// Load a binary and start
std::string errorMsg;
if (emulator.Init("program.bin", errorMsg)) {
    emulator.Start();    // Starts the emulation thread
}

emulator.Pause();        // Pauses execution
emulator.Resume();       // Resumes execution
emulator.Stop();         // Stops the thread
```

### Internal components

| Field  | Type             | Description                             |
| ------ | ---------------- | --------------------------------------- |
| `bus`  | `Hardware::Bus`  | Memory bus (RAM + ROM + Device mapping) |
| `cpu`  | `Hardware::CPU`  | 65c02 processor core                    |
| `ram`  | `Hardware::RAM`  | 32 KB RAM                               |
| `rom`  | `Hardware::ROM`  | 32 KB ROM                               |
| `lcd`  | `Hardware::LCD`  | 2×16 LCD display                        |
| `acia` | `Hardware::ACIA` | Serial interface (6551)                 |
| `gpu`  | `Hardware::GPU`  | Graphics processor + VRAM               |
| `sid`  | `Hardware::SID`  | Sound chip                              |
| `via`  | `Hardware::VIA`  | Versatile I/O interface (6522)          |
| `sd`   | `Hardware::SDCard` | SD Card emulation (SPI)                 |
| `esp8266` | `Hardware::ESP8266` | Network chip (Wi-Fi simulation)         |

### Emulation thread

The emulator runs on a dedicated `std::thread`. Speed is controlled with `SetTargetIPS(int ips)` (instructions per second). The actual measured speed can be queried with `GetActualIPS()`.

```cpp
emulator.SetTargetIPS(1'000'000);   // 1 MHz
int actual = emulator.GetActualIPS();
```

Thread synchronization between the emulation thread and the GUI uses:

- `std::atomic<bool>` for `running` and `paused`
- `std::condition_variable` (`pauseCV`) for efficient wait during pause
- `std::mutex emulationMutex` for synchronization during state changes (reset/step/saveload)

### Step-by-step mode

```cpp
int result = emulator.Step();
// 0  → OK
// 1  → STOP/JAM
// -1 → Invalid opcode
```

### Keyboard input

Keys are injected into the input buffer, which the CPU reads through the ACIA:

```cpp
emulator.InjectKey('A');
```

### Output callbacks

```cpp
emulator.SetOutputCallback([](char c) {
    std::cout << c;          // Serial output → console
});
emulator.SetLCDOutputCallback([](char c) {
    // Each character written to the LCD
});
```

---

## `Bus` class — Memory system and Device Orchestration

The `Bus` class (formerly `Mem`) manages the 64KB address space and handles communication between the CPU and all connected peripherals.

**File:** `Hardware/Core/Bus.h` / `Hardware/Core/Bus.cpp`

### IBusDevice Interface

All hardware components connected to the bus must implement the `IBusDevice` interface. This ensures a consistent way to handle memory mapping and state serialization.

```cpp
class IBusDevice {
public:
    virtual std::string GetName() const = 0;

    // Savestate support
    virtual bool SaveState(std::ostream& out) const = 0;
    virtual bool LoadState(std::istream& in) = 0;
};
```

### Memory map (Default)

```
0x0000 – 0x7FFF   RAM (32 KB)
0x2000 – 0x3FFF   VRAM (7.5 KB) — only when GPU is enabled
0x4800 – 0x481F   SID (Sound chip registers)
0x5000 – 0x5003   ACIA (serial communication registers)
0x5004 – 0x5007   ESP8266 (Network chip registers)
0x6000 – 0x600F   VIA (I/O registers)
0x8000 – 0xFFFF   ROM (32 KB, read-only)
0xFFFC – 0xFFFD   RESET vector
0xFFFE – 0xFFFF   IRQ/BRK vector
```

> **Note on Virtual Devices:** Components like the **LCD** and **SD Card** are not directly mapped to memory addresses. Instead, they are controlled via the **VIA Port B** (`0x6000`):
> - **LCD**: Receives commands and data directly through Port B pins.
> - **SD Card**: Uses Port B pins for SPI bit-banging: `PB0` (MOSI), `PB1` (MISO), `PB2` (CLK), `PB3` (CS).
>
> ROM occupies the upper half of the address space. Write attempts to `addr >= 0x8000` are silently ignored.

### Read/write hooks

The bus supports per-address _hooks_ or _device registration_, allowing peripherals (ACIA, VIA, GPU) to intercept accesses:

```cpp
bus.RegisterDevice(myDevice, 0x5000, 0x5003);
```

Hook types:

```cpp
using WriteHook = void (*)(void*, Word, Byte);
using ReadHook  = Byte (*)(void*, Word);
```

### Memory-mapped peripheral registers

| Address           | Register             | Peripheral             |
| ----------------- | -------------------- | ---------------------- |
| `0x5000`          | `ACIA_DATA`          | ACIA — serial data     |
| `0x5001`          | `ACIA_STATUS`        | ACIA — status          |
| `0x5002`          | `ACIA_CMD`           | ACIA — command         |
| `0x5003`          | `ACIA_CTRL`          | ACIA — control         |
| `0x6000`          | `PORTB`              | VIA — Port B           |
| `0x6001`          | `PORTA`              | VIA — Port A           |
| `0x6002`          | `DDRB`               | VIA — Data Direction B |
| `0x6003`          | `DDRA`               | VIA — Data Direction A |
| `0x6004`–`0x6009` | `T1C/T2C`            | VIA — Timers           |
| `0x600A`–`0x600F` | `SR/ACR/PCR/IFR/IER` | VIA — Control          |

### Low-level API

```cpp
Byte val = bus.Read(0x0200);
bus.Write(0x0200, 0x42);
bus.WriteWord(0x1234, 0x0300);   // Write word in little-endian
bus.WriteROM(0x8000, 0xEA);      // For unit tests only
```
