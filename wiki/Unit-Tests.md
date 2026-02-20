# Unit Tests — Unit Test Suite

**Directory:** `UnitTests/`  
**Framework:** [Google Test](https://github.com/google/googletest)

## Overview

The `UnitTests/` directory contains a unit test suite that verifies the correct behaviour of all 65c02 processor instructions and some additional hardware components.

> ⚠️ The `UnitTests/` folder was generated with AI assistance.

## Structure

Each instruction has its own test file or directory:

```
UnitTests/
├── ADC/              ← ADC tests (multiple files)
├── AND/
├── ASL/
├── BCC.cpp
├── BCS.cpp
├── BEQ.cpp
├── ...               ← One .cpp per branch instruction
├── CMP/
├── SBC/
├── STA/
├── ...
├── Ampliados/        ← Tests for extended 65C02 instructions
├── LCD_Test.cpp      ← LCD tests (in some setups)
├── VIA_Test.cpp      ← VIA 6522 tests
└── repro_freeze.cpp  ← Reproduction of a freeze bug
```

## Test pattern

All tests follow the same pattern using **Google Test fixtures**:

```cpp
#include <gtest/gtest.h>
#include "../Hardware/CPU.h"
#include "../Hardware/CPU/Instructions/InstructionSet.h"
#include "../Hardware/Mem.h"

using namespace Hardware;

class BEQ_Test : public ::testing::Test {
protected:
    void SetUp() override { cpu.Reset(mem); }
    Mem mem;
    CPU cpu;
};

// Test BEQ when Z=0 (no branch)
TEST_F(BEQ_Test, BEQ_NoBranch_ZeroClear) {
    cpu.Z = 0;
    cpu.PC = 0x1000;
    mem.Write(0x1000, INS_BEQ);
    mem.Write(0x1001, 0x05);    // offset +5
    mem.WriteROM(0xFFFC, 0x00);
    mem.WriteROM(0xFFFD, 0x10);
    cpu.Execute(mem);
    EXPECT_EQ(cpu.PC, 0x1003);  // Did not branch: PC advanced normally
}

// Test BEQ when Z=1 (branch taken)
TEST_F(BEQ_Test, BEQ_Branch_ZeroSet) {
    cpu.Z = 1;
    cpu.PC = 0x1000;
    mem.Write(0x1000, INS_BEQ);
    mem.Write(0x1001, 0x05);    // offset +5
    mem.WriteROM(0xFFFC, 0x00);
    mem.WriteROM(0xFFFD, 0x10);
    cpu.Execute(mem);
    EXPECT_EQ(cpu.PC, 0x1008);  // Branched: 0x1002 + 5 = 0x1007 (JAM); fetch advances PC to 0x1008
}
```

### Termination technique (`INS_JAM`)

Tests use the `JAM` instruction (also known as `STP` or `KIL`) as a **terminator**: when executed, `cpu.Execute()` returns 1 and the test can verify the final register state.

## Instructions with tests

### Simple tests (one `.cpp` file each)

| Instruction | File |
|-------------|------|
| BBR0–BBR7 | `BBR.cpp` |
| BBS0–BBS7 | `BBS.cpp` |
| BCC | `BCC.cpp` |
| BCS | `BCS.cpp` |
| BEQ | `BEQ.cpp` |
| BMI | `BMI.cpp` |
| BNE | `BNE.cpp` |
| BPL | `BPL.cpp` |
| BRA | `BRA.cpp` |
| BRK | `BRK.cpp` |
| BVC | `BVC.cpp` |
| BVS | `BVS.cpp` |
| CLC, CLD, CLI, CLV | `CLC.cpp`, `CLD.cpp`, `CLI.cpp`, `CLV.cpp` |
| DEX, DEY | `DEX.cpp`, `DEY.cpp` |
| INX, INY | `INX.cpp`, `INY.cpp` |
| NOP | `NOP.cpp` |
| PHA, PHP, PHX, PHY | `PHA.cpp`, `PHP.cpp`, `PHX.cpp`, `PHY.cpp` |
| PLA, PLP, PLX, PLY | `PLA.cpp`, `PLP.cpp`, `PLX.cpp`, `PLY.cpp` |
| RMB, SMB | `RMB.cpp`, `SMB.cpp` |
| RTI, RTS | `RTI.cpp`, `RTS.cpp` |
| SEC, SED, SEI | `SEC.cpp`, `SED.cpp`, `SEI.cpp` |
| TAX, TAY, TSX, TXA, TXS, TYA | Individual `.cpp` files |

### Directory tests (multiple addressing modes)

| Instruction | Directory | Modes tested |
|-------------|-----------|--------------|
| ADC | `ADC/` | Immediate, ZP, ZP_X, Absolute, Absolute_X, Absolute_Y, Indirect_X, Indirect_Y |
| AND | `AND/` | All modes |
| ASL | `ASL/` | Accumulator, ZP, Absolute, etc. |
| CMP | `CMP/` | All modes |
| CPX, CPY | `CPX/`, `CPY/` | Immediate, ZP, Absolute |
| DEC, INC | `DEC/`, `INC/` | ZP, Absolute, Accumulator |
| EOR | `EOR/` | All modes |
| LDA, LDX, LDY | `LDA/`, `LDX/`, `LDY/` | All modes |
| ROL, ROR | `ROL/`, `ROR/` | Accumulator, ZP, Absolute |
| SBC | `SBC/` | All modes |
| STA, STX, STY, STZ | `STA/`, `STX/`, `STY/`, `STZ/` | All modes |
| TRB, TSB | `TRB/`, `TSB/` | ZP, Absolute |

### Hardware tests

| File | Description |
|------|-------------|
| `VIA_Test.cpp` | Tests T1/T2 timers, DDR registers, and VIA 6522 callbacks |
| `repro_freeze.cpp` | Reproduction of an emulator freeze case |

## Running the tests

Tests are compiled and run automatically at the end of `build-linux.sh`:

```bash
./build-linux.sh
```

To run the tests manually after building:

```bash
./output/linux/unit_tests
# Or with a filter for a specific instruction:
./output/linux/unit_tests --gtest_filter="BEQ*"
```

## CMake integration

Tests are registered as the `unit_tests` target in `CMakeLists.txt` and linked against the `65c02_core` library and `googletest`.
