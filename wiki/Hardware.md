# Hardware — Main emulator orchestrator and memory system

## `Emulator` class

`Core::Emulator` is the **central orchestrator** of the simulator. It coordinates all hardware components, manages the emulation thread, and exposes a clean API for the graphical interface.

**File:** `Hardware/Emulator.h` / `Hardware/Emulator.cpp`

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

| Field | Type | Description |
|-------|------|-------------|
| `mem` | `Hardware::Mem` | Memory system (RAM + ROM + hooks) |
| `cpu` | `Hardware::CPU` | 65c02 processor core |
| `lcd` | `Hardware::LCD` | 2×16 LCD display |
| `acia` | `Hardware::ACIA` | Serial interface (6551) |
| `gpu` | `Hardware::GPU` | Graphics processor + VRAM |
| `sid` | `Hardware::SID` | Sound chip |
| `via` | `Hardware::VIA` | Versatile I/O interface (6522) |

### Emulation thread

The emulator runs on a dedicated `std::thread`. Speed is controlled with `SetTargetIPS(int ips)` (instructions per second). The actual measured speed can be queried with `GetActualIPS()`.

```cpp
emulator.SetTargetIPS(1'000'000);   // 1 MHz
int actual = emulator.GetActualIPS();
```

Thread synchronization between the emulation thread and the GUI uses:
- `std::atomic<bool>` for `running` and `paused`
- `std::condition_variable` (`pauseCV`) for efficient wait during pause
- `std::mutex emulationMutex` for atomic reset/step operations

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

## `Mem` class — Memory system

**File:** `Hardware/Mem.h` / `Hardware/Mem.cpp`

### Memory map

```
0x0000 – 0x7FFF   RAM (32 KB)
0x2000 – 0x3FFF   VRAM (7.5 KB) — only when GPU is enabled
0x5000 – 0x5003   ACIA (serial communication registers)
0x6000 – 0x600F   VIA (I/O registers)
0x8000 – 0xFFFF   ROM (32 KB, read-only)
0xFFFC – 0xFFFD   RESET vector
0xFFFE – 0xFFFF   IRQ/BRK vector
```

> ROM occupies the upper half of the address space. Write attempts to `addr >= 0x8000` are silently ignored.

### Read/write hooks

The memory system supports per-address *hooks*, allowing peripherals (ACIA, VIA, GPU) to intercept accesses:

```cpp
mem.SetWriteHook(0x5000, myWriteHook, contextPtr);
mem.SetReadHook(0x5001, myReadHook, contextPtr);
```

Hook types:
```cpp
using WriteHook = void (*)(void*, Word, Byte);
using ReadHook  = Byte (*)(void*, Word);
```

### Memory-mapped peripheral registers

| Address | Register | Peripheral |
|---------|----------|------------|
| `0x5000` | `ACIA_DATA` | ACIA — serial data |
| `0x5001` | `ACIA_STATUS` | ACIA — status |
| `0x5002` | `ACIA_CMD` | ACIA — command |
| `0x5003` | `ACIA_CTRL` | ACIA — control |
| `0x6000` | `PORTB` | VIA — Port B |
| `0x6001` | `PORTA` | VIA — Port A |
| `0x6002` | `DDRB` | VIA — Data Direction B |
| `0x6003` | `DDRA` | VIA — Data Direction A |
| `0x6004`–`0x6009` | `T1C/T2C` | VIA — Timers |
| `0x600A`–`0x600F` | `SR/ACR/PCR/IFR/IER` | VIA — Control |

### Low-level API

```cpp
Byte val = mem.Read(0x0200);
mem.Write(0x0200, 0x42);
mem.WriteWord(0x1234, 0x0300);   // Write word in little-endian
mem.WriteROM(0x8000, 0xEA);      // For unit tests only
```
