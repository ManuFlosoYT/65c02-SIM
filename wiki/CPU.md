# CPU — WDC 65C02 Processor

**File:** `Hardware/CPU.h` / `Hardware/CPU.cpp`  
**Namespace:** `Hardware::CPU`

## Overview

The emulation core implements the **WDC 65C02** processor, the enhanced CMOS variant of the classic MOS 6502. It adds new instructions (bit manipulation, `PHX`, `PHY`, `BRA`, etc.) and fixes several behaviors from the original NMOS 6502.

## Registers

| Register | Size | Description |
|----------|------|-------------|
| `PC` | 16 bits | Program Counter — address of the next instruction |
| `SP` | 16 bits | Stack Pointer — points into page 1 (`0x0100`–`0x01FF`) |
| `A` | 8 bits | Accumulator |
| `X` | 8 bits | X index register |
| `Y` | 8 bits | Y index register |

## Status register (flags)

The status byte is built with `GetStatus()` / `SetStatus()`:

| Bit | Flag | Description |
|-----|------|-------------|
| 7 | `N` | Negative (bit 7 of the result) |
| 6 | `V` | Overflow |
| 5 | — | Always 1 (not implemented as a field) |
| 4 | `B` | Break — indicates interrupt source on the stack |
| 3 | `D` | BCD (Decimal) mode |
| 2 | `I` | IRQ interrupt disable |
| 1 | `Z` | Zero (result was 0) |
| 0 | `C` | Carry |

## Interrupt vectors

| Address | Vector | Use |
|---------|--------|-----|
| `0xFFFA`–`0xFFFB` | NMI | Non-maskable interrupt |
| `0xFFFC`–`0xFFFD` | RESET | Start address after reset |
| `0xFFFE`–`0xFFFF` | IRQ/BRK | Maskable interrupt / BRK instruction |

On startup, the CPU reads the RESET vector (`0xFFFC`) to obtain the initial address.

## Execution cycle

```
Reset() → isInit = false
   ↓
Step() → if !isInit: read RESET vector → PC
   ↓
Dispatch() → fetch opcode at PC, execute instruction
   ↓
Returns 0 (OK) / 1 (JAM) / -1 (invalid opcode)
```

### Cycle-accurate mode (`cycleAccurate`)

When enabled, each instruction consumes the correct number of clock cycles:

```cpp
emulator.SetCycleAccurate(true);
```

Internally, `remainingCycles` decrements on each `Step()` call until it reaches 0, at which point the next instruction is executed. Page-crossing penalties are also applied:

```cpp
cpu.AddPageCrossPenalty(baseAddr, effectiveAddr);
```

## Interrupts

### IRQ (maskable)
```
IRQ() → if flag I == 0:
   1. Push PC onto stack
   2. Push status (B=0) onto stack
   3. I = 1, D = 0  ← the 65C02 clears the Decimal flag on IRQ
   4. PC = mem[0xFFFE]
```

### Wait mode (`WAI`)
The `WAI` instruction puts the CPU into `waiting = true` state. It wakes up when bit 7 of the `ACIA_STATUS` register (`0x5001`) becomes 1.

## Stack

The stack lives in **page 1** (`0x0100`–`0x01FF`). The stack pointer (`SP`) points to the next free byte and grows downward. Overflow wraps within the same page:

```
PushByte: SP < 0x0100 → SP = 0x01FF
PopByte:  SP > 0x01FF → SP = 0x0100
```

## Instruction dispatch

Dispatch is handled in `Hardware/CPU/Dispatch.cpp`. Each opcode has its own class in `Hardware/CPU/Instructions/`, allowing the compiler to apply inline expansion or LTO.

```
Dispatch(cpu, mem)
   └─ FetchByte(PC)  →  opcode
      └─ switch(opcode)  →  Instruction::Execute(cpu, mem)
```

See the [CPU Instructions](CPU-Instructions) page for the full list of supported instructions.
