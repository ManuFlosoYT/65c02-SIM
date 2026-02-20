# CPU Instructions — 65C02 Instruction Set

The emulator implements the complete instruction set of the **WDC 65C02**, including the additional instructions compared to the original NMOS 6502.

Each instruction has its own class in `Hardware/CPU/Instructions/`. Dispatch is performed via a `switch` table in `Hardware/CPU/Dispatch.cpp`.

## Instructions by category

### Arithmetic

| Mnemonic | Description | File |
|----------|-------------|------|
| `ADC` | Add with Carry — adds to the accumulator with carry | `ADC.h/cpp` |
| `SBC` | Subtract with Carry — subtracts from the accumulator with carry | `SBC.h/cpp` |
| `INC` | Increment — increments memory or accumulator | `INC.h/cpp` |
| `DEC` | Decrement — decrements memory or accumulator | `DEC.h/cpp` |
| `INX` | Increment X | `INX.h/cpp` |
| `INY` | Increment Y | `INY.h/cpp` |
| `DEX` | Decrement X | `DEX.h/cpp` |
| `DEY` | Decrement Y | `DEY.h/cpp` |

### Logic and shifts

| Mnemonic | Description | File |
|----------|-------------|------|
| `AND` | Logical AND with accumulator | `AND.h/cpp` |
| `ORA` | Inclusive OR with accumulator | `ORA.h/cpp` |
| `EOR` | Exclusive OR (XOR) with accumulator | `EOR.h/cpp` |
| `ASL` | Arithmetic Shift Left | `ASL.h/cpp` |
| `LSR` | Logical Shift Right | `LSR.h/cpp` |
| `ROL` | Rotate Left (through Carry) | `ROL.h/cpp` |
| `ROR` | Rotate Right (through Carry) | `ROR.h/cpp` |

### Load and store

| Mnemonic | Description | File |
|----------|-------------|------|
| `LDA` | Load Accumulator | `LDA.h/cpp` |
| `LDX` | Load X | `LDX.h/cpp` |
| `LDY` | Load Y | `LDY.h/cpp` |
| `STA` | Store Accumulator | `STA.h/cpp` |
| `STX` | Store X | `STX.h/cpp` |
| `STY` | Store Y | `STY.h/cpp` |
| `STZ` | Store Zero (65C02) | `STZ.h/cpp` |

### Register transfers

| Mnemonic | Description | File |
|----------|-------------|------|
| `TAX` | Transfer A → X | `TAX.h/cpp` |
| `TAY` | Transfer A → Y | `TAY.h/cpp` |
| `TXA` | Transfer X → A | `TXA.h/cpp` |
| `TYA` | Transfer Y → A | `TYA.h/cpp` |
| `TSX` | Transfer SP → X | `TSX.h/cpp` |
| `TXS` | Transfer X → SP | `TXS.h/cpp` |

### Stack

| Mnemonic | Description | File |
|----------|-------------|------|
| `PHA` | Push Accumulator | `PHA.h/cpp` |
| `PLA` | Pull Accumulator | `PLA.h/cpp` |
| `PHP` | Push Processor status | `PHP.h/cpp` |
| `PLP` | Pull Processor status | `PLP.h/cpp` |
| `PHX` | Push X (65C02) | `PHX.h/cpp` |
| `PLX` | Pull X (65C02) | `PLX.h/cpp` |
| `PHY` | Push Y (65C02) | `PHY.h/cpp` |
| `PLY` | Pull Y (65C02) | `PLY.h/cpp` |

### Branches (conditional jumps)

| Mnemonic | Condition | File |
|----------|-----------|------|
| `BCC` | Branch if Carry Clear (C=0) | `BCC.h/cpp` |
| `BCS` | Branch if Carry Set (C=1) | `BCS.h/cpp` |
| `BEQ` | Branch if Equal / Zero Set (Z=1) | `BEQ.h/cpp` |
| `BNE` | Branch if Not Equal / Zero Clear (Z=0) | `BNE.h/cpp` |
| `BMI` | Branch if Minus (N=1) | `BMI.h/cpp` |
| `BPL` | Branch if Plus (N=0) | `BPL.h/cpp` |
| `BVC` | Branch if Overflow Clear (V=0) | `BVC.h/cpp` |
| `BVS` | Branch if Overflow Set (V=1) | `BVS.h/cpp` |
| `BRA` | Branch Always (65C02) | `BRA.h/cpp` |

### Flow control

| Mnemonic | Description | File |
|----------|-------------|------|
| `JMP` | Jump — unconditional jump | `JMP.h/cpp` |
| `JSR` | Jump to Subroutine | `JSR.h/cpp` |
| `RTS` | Return from Subroutine | `RTS.h/cpp` |
| `RTI` | Return from Interrupt | `RTI.h/cpp` |
| `BRK` | Break — software interrupt | `BRK.h/cpp` |
| `NOP` | No Operation | `NOP.h/cpp` |

### Status flags

| Mnemonic | Description | File |
|----------|-------------|------|
| `CLC` | Clear Carry | `CLC.h/cpp` |
| `SEC` | Set Carry | `SEC.h/cpp` |
| `CLI` | Clear Interrupt Disable | `CLI.h/cpp` |
| `SEI` | Set Interrupt Disable | `SEI.h/cpp` |
| `CLD` | Clear Decimal | `CLD.h/cpp` |
| `SED` | Set Decimal | `SED.h/cpp` |
| `CLV` | Clear Overflow | `CLV.h/cpp` |

### Comparisons

| Mnemonic | Description | File |
|----------|-------------|------|
| `CMP` | Compare Accumulator | `CMP.h/cpp` |
| `CPX` | Compare X | `CPX.h/cpp` |
| `CPY` | Compare Y | `CPY.h/cpp` |
| `BIT` | Bit Test | `BIT.h/cpp` |

### Bit manipulation (65C02)

Instructions exclusive to the WDC 65C02 for operating on individual bits in zero page:

| Mnemonic | Description | File |
|----------|-------------|------|
| `BBR` | Branch if Bit Reset — branches if bit N of the zero-page address is 0 | `BBR.h/cpp` |
| `BBS` | Branch if Bit Set — branches if bit N of the zero-page address is 1 | `BBS.h/cpp` |
| `RMB` | Reset Memory Bit — clears bit N of the zero-page address | `RMB.h/cpp` |
| `SMB` | Set Memory Bit — sets bit N of the zero-page address | `SMB.h/cpp` |
| `TSB` | Test and Set Bits | `TSB.h/cpp` |
| `TRB` | Test and Reset Bits | `TRB.h/cpp` |

## Supported addressing modes

| Mode | Example | Description |
|------|---------|-------------|
| Implied | `CLC` | No operand |
| Accumulator | `ASL A` | Operates on the accumulator |
| Immediate | `LDA #$42` | Constant value |
| Zero Page | `LDA $10` | Address in `0x0000`–`0x00FF` |
| Zero Page, X | `LDA $10,X` | Zero page indexed by X |
| Zero Page, Y | `LDA $10,Y` | Zero page indexed by Y |
| Absolute | `LDA $1234` | 16-bit address |
| Absolute, X | `LDA $1234,X` | Absolute indexed by X |
| Absolute, Y | `LDA $1234,Y` | Absolute indexed by Y |
| Indirect | `JMP ($1234)` | Indirection through pointer |
| (Indirect, X) | `LDA ($10,X)` | Pre-indexed indirect by X |
| (Indirect), Y | `LDA ($10),Y` | Post-indexed indirect by Y |
| (Indirect) Zero Page | `LDA ($10)` | Zero-page indirect (65C02) |
| Relative | `BEQ label` | 8-bit relative offset |
| Absolute Indirect X | `JMP ($1234,X)` | Absolute indexed indirect (65C02) |

## Cycle accuracy

When cycle-accurate mode is enabled (`SetCycleAccurate(true)`), each instruction consumes the correct number of 65C02 clock cycles. Accesses that cross a page boundary add an extra penalty cycle.
