# CPU Instructions — 65C02 Instruction Set Reference

This document provides a comprehensive technical reference for the complete instruction set of the **Western Design Center (WDC) 65C02** microprocessor as implemented in **65c02-SIM**.

Every opcode is implemented as an isolated, modular class inside [`src/Hardware/CPU/Instructions/`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Hardware/CPU/Instructions/). All instruction handlers are templated on `<bool Debug>` to allow zero-cost compilation when profiling and debugger hooks are inactive.

---

## Addressing Modes (16 Modes)

The 65C02 expands the NMOS 6502 addressing model by adding Zero Page Indirect, Absolute Indexed Indirect, and Zero Page Relative modes, bringing the total to **16 distinct addressing modes**:

| Addressing Mode | Mnemonic Syntax | Bytes | Effective Address Calculation | Description |
| :--- | :--- | :---: | :--- | :--- |
| **Implied** | `NOP`, `CLC` | 1 | None | Operand is implied by the instruction opcode. |
| **Accumulator** | `ASL A`, `INC A` | 1 | Register `A` | Operates directly on the Accumulator. |
| **Immediate** | `LDA #$42` | 2 | `PC + 1` | 8-bit literal constant stored in the byte following the opcode. |
| **Zero Page** | `LDA $10` | 2 | `$0000 + Byte` | 8-bit address in Page 0 (`$0000`–`$00FF`). Fastest memory access mode. |
| **Zero Page, X** | `LDA $10,X` | 2 | `$0000 + (Byte + X) & 0xFF` | Zero Page address indexed by X with 8-bit wraparound inside Page 0. |
| **Zero Page, Y** | `LDX $10,Y` | 2 | `$0000 + (Byte + Y) & 0xFF` | Zero Page address indexed by Y with 8-bit wraparound (used by `LDX`, `STX`). |
| **Absolute** | `LDA $1234` | 3 | `Word` | Direct 16-bit memory address (`$0000`–`$FFFF`) stored in little-endian order. |
| **Absolute, X** | `LDA $1234,X` | 3 | `Word + X` | 16-bit address indexed by X. Adds 1 cycle penalty if a page boundary is crossed. |
| **Absolute, Y** | `LDA $1234,Y` | 3 | `Word + Y` | 16-bit address indexed by Y. Adds 1 cycle penalty if a page boundary is crossed. |
| **Indirect** | `JMP ($1234)` | 3 | `mem[Word] \| (mem[Word+1] << 8)` | 16-bit pointer to target address. NMOS page-boundary bug is fixed. |
| **Indexed Indirect (X)** | `LDA ($10,X)` | 2 | `addr = (Byte + X) & 0xFF; mem[addr] \| (mem[addr+1] << 8)` | Pre-indexed indirect: X is added to zero-page pointer address with wraparound. |
| **Indirect Indexed (Y)** | `LDA ($10),Y` | 2 | `base = mem[Byte] \| (mem[Byte+1] << 8); base + Y` | Post-indexed indirect: 16-bit pointer resolved from Zero Page, then Y is added. |
| **Relative** | `BEQ $05` | 2 | `PC + 2 + signed_offset` | Signed 8-bit branch offset (-128 to +127 bytes from instruction end). |
| **Zero Page Indirect** | `LDA ($10)` | 2 | `mem[Byte] \| (mem[Byte+1] << 8)` | **65C02 Extension**: Resolves 16-bit target address directly from Zero Page. |
| **Absolute Indexed Indirect** | `JMP ($1234,X)` | 3 | `ptr = Word + X; mem[ptr] \| (mem[ptr+1] << 8)` | **65C02 Extension**: Jump table vector lookup indexed by X. |
| **Zero Page Relative** | `BBR0 $10, label` | 3 | `zp = Byte1; target = PC + 3 + signed_offset` | **65C02 Extension**: Tests bit in Zero Page and performs relative branch. |

---

## CMOS Instruction Extensions

The WDC 65C02 adds significant computing capability over the classic NMOS 6502:

### 1. Unconditional Branch (`BRA`)
- **Syntax:** `BRA label` (Opcode `$80`, 2 bytes, Relative mode)
- **Base Cycles:** 3 (+1 if branch crosses a page boundary)
- Replaces awkward unconditional jump constructs (`JMP` or `CLC : BCC`) with relocatable relative code.

### 2. Store Zero (`STZ`)
- **Syntax:** `STZ zp` (`$64`), `STZ zp,X` (`$74`), `STZ abs` (`$9C`), `STZ abs,X` (`$9E`)
- Writes `0x00` directly to memory without clobbering the Accumulator, X, or Y registers.
- Flags affected: None.

### 3. Bit Manipulation: Test & Reset/Set Bits (`TRB` / `TSB`)
- **Syntax:**
  - `TRB zp` (`$14`), `TRB abs` (`$1C`) — Test and Reset Bits
  - `TSB zp` (`$04`), `TSB abs` (`$0C`) — Test and Set Bits
- **Operation:**
  1. Performs bitwise `A & mem` to evaluate the Zero flag: `Z = ((A & mem) == 0)`.
  2. For `TRB`: writes `(~A) & mem` back to memory (clears memory bits that are 1 in `A`).
  3. For `TSB`: writes `A | mem` back to memory (sets memory bits that are 1 in `A`).
- Flags affected: `Z`.

### 4. Zero Page Single-Bit Set / Reset (`SMB0`–`SMB7` / `RMB0`–`RMB7`)
- **Opcodes:**
  - `RMB0`–`RMB7`: `$07`, `$17`, `$27`, `$37`, `$47`, `$57`, `$67`, `$77` (2 bytes, 5 cycles)
  - `SMB0`–`SMB7`: `$87`, `$97`, `$A7`, `$B7`, `$C7`, `$D7`, `$E7`, `$F7` (2 bytes, 5 cycles)
- **Operation:** Atomically sets or resets bit *n* (0–7) of the target Zero Page memory location without needing read-modify-write sequences in registers.
- Flags affected: None.

### 5. Zero Page Branch on Bit Set / Reset (`BBS0`–`BBS7` / `BBR0`–`BBR7`)
- **Opcodes:**
  - `BBR0`–`BBR7`: `$0F`, `$1F`, `$2F`, `$3F`, `$4F`, `$5F`, `$6F`, `$7F` (3 bytes, 5 cycles base)
  - `BBS0`–`BBS7`: `$8F`, `$9F`, `$AF`, `$BF`, `$CF`, `$DF`, `$EF`, `$FF` (3 bytes, 5 cycles base)
- **Syntax:** `BBR3 $10, target_label`
- **Operation:** Reads zero-page byte `$10`. If bit 3 is 0 (for `BBR`) or 1 (for `BBS`), branches to `target_label`.
- **Timing:** 5 cycles base; +1 cycle if branch is taken; +1 additional cycle if branch crosses a page boundary.

### 6. Stack Register Push / Pull (`PHX`, `PLX`, `PHY`, `PLY`)
- **Opcodes:**
  - `PHX` (`$DA`, 3 cycles), `PLX` (`$FA`, 4 cycles)
  - `PHY` (`$5A`, 3 cycles), `PLY` (`$7A`, 4 cycles)
- Enables saving and restoring index registers directly without routing through the Accumulator.
- `PLX` and `PLY` update the `N` and `Z` flags based on the pulled value.

### 7. Accumulator Increment / Decrement (`INC A` / `DEC A`)
- **Opcodes:** `INC A` (`$1A`, 2 cycles), `DEC A` (`$3A`, 2 cycles)
- Direct single-byte register arithmetic on `A` without needing `ADC #1` or `SEC : SBC #1`.
- Flags affected: `N`, `Z`.

### 8. System State Control (`WAI` / `STP`)
- **`WAI` (Wait for Interrupt — `$CB`, 3 cycles)**:
  Puts the CPU into low-power halt mode (`cpu.waiting = true`). Memory bus activity stops until a hardware `IRQ` or `NMI` signal is received.
- **`STP` (Stop the Processor — `$DB`, 3 cycles)**:
  Shuts down the processor completely. Emulation stops with `exitCode = 1`. Can only be restarted by resetting the hardware.

---

## Instruction Set Catalog

### 1. Load, Store, and Transfer

| Mnemonic | Opcode | Mode | Bytes | Cycles | Flags (N V - B D I Z C) | Description |
| :--- | :---: | :--- | :---: | :---: | :---: | :--- |
| **LDA** | `$A9` | Immediate | 2 | 2 | `N . . . . . Z .` | Load Accumulator with constant |
| | `$A5` | Zero Page | 2 | 3 | `N . . . . . Z .` | Load Accumulator from Zero Page |
| | `$B5` | Zero Page, X | 2 | 4 | `N . . . . . Z .` | Load Accumulator from Zero Page indexed by X |
| | `$AD` | Absolute | 3 | 4 | `N . . . . . Z .` | Load Accumulator from Absolute address |
| | `$BD` | Absolute, X | 3 | 4* | `N . . . . . Z .` | Load Accumulator from Absolute indexed by X (*+1 on page cross) |
| | `$B9` | Absolute, Y | 3 | 4* | `N . . . . . Z .` | Load Accumulator from Absolute indexed by Y (*+1 on page cross) |
| | `$A1` | (Indirect, X) | 2 | 6 | `N . . . . . Z .` | Load Accumulator from Indexed Indirect |
| | `$B1` | (Indirect), Y | 2 | 5* | `N . . . . . Z .` | Load Accumulator from Indirect Indexed (*+1 on page cross) |
| | `$B2` | (Indirect) ZP | 2 | 5 | `N . . . . . Z .` | Load Accumulator from Zero Page Indirect (65C02) |
| **LDX** | `$A2` | Immediate | 2 | 2 | `N . . . . . Z .` | Load X Register with constant |
| | `$A6` | Zero Page | 2 | 3 | `N . . . . . Z .` | Load X Register from Zero Page |
| | `$B6` | Zero Page, Y | 2 | 4 | `N . . . . . Z .` | Load X Register from Zero Page indexed by Y |
| | `$AE` | Absolute | 3 | 4 | `N . . . . . Z .` | Load X Register from Absolute address |
| | `$BE` | Absolute, Y | 3 | 4* | `N . . . . . Z .` | Load X Register from Absolute indexed by Y (*+1 on page cross) |
| **LDY** | `$A0` | Immediate | 2 | 2 | `N . . . . . Z .` | Load Y Register with constant |
| | `$A4` | Zero Page | 2 | 3 | `N . . . . . Z .` | Load Y Register from Zero Page |
| | `$B4` | Zero Page, X | 2 | 4 | `N . . . . . Z .` | Load Y Register from Zero Page indexed by X |
| | `$AC` | Absolute | 3 | 4 | `N . . . . . Z .` | Load Y Register from Absolute address |
| | `$BC` | Absolute, X | 3 | 4* | `N . . . . . Z .` | Load Y Register from Absolute indexed by X (*+1 on page cross) |
| **STA** | `$85` | Zero Page | 2 | 3 | `. . . . . . . .` | Store Accumulator to Zero Page |
| | `$95` | Zero Page, X | 2 | 4 | `. . . . . . . .` | Store Accumulator to Zero Page indexed by X |
| | `$8D` | Absolute | 3 | 4 | `. . . . . . . .` | Store Accumulator to Absolute address |
| | `$9D` | Absolute, X | 3 | 5 | `. . . . . . . .` | Store Accumulator to Absolute indexed by X |
| | `$99` | Absolute, Y | 3 | 5 | `. . . . . . . .` | Store Accumulator to Absolute indexed by Y |
| | `$81` | (Indirect, X) | 2 | 6 | `. . . . . . . .` | Store Accumulator to Indexed Indirect |
| | `$91` | (Indirect), Y | 2 | 6 | `. . . . . . . .` | Store Accumulator to Indirect Indexed |
| | `$92` | (Indirect) ZP | 2 | 5 | `. . . . . . . .` | Store Accumulator to Zero Page Indirect (65C02) |
| **STX** | `$86` | Zero Page | 2 | 3 | `. . . . . . . .` | Store X Register to Zero Page |
| | `$96` | Zero Page, Y | 2 | 4 | `. . . . . . . .` | Store X Register to Zero Page indexed by Y |
| | `$8E` | Absolute | 3 | 4 | `. . . . . . . .` | Store X Register to Absolute address |
| **STY** | `$84` | Zero Page | 2 | 3 | `. . . . . . . .` | Store Y Register to Zero Page |
| | `$94` | Zero Page, X | 2 | 4 | `. . . . . . . .` | Store Y Register to Zero Page indexed by X |
| | `$8C` | Absolute | 3 | 4 | `. . . . . . . .` | Store Y Register to Absolute address |
| **STZ** | `$64` | Zero Page | 2 | 3 | `. . . . . . . .` | Store Zero ($00) to Zero Page (65C02) |
| | `$74` | Zero Page, X | 2 | 4 | `. . . . . . . .` | Store Zero ($00) to Zero Page indexed by X (65C02) |
| | `$9C` | Absolute | 3 | 4 | `. . . . . . . .` | Store Zero ($00) to Absolute address (65C02) |
| | `$9E` | Absolute, X | 3 | 5 | `. . . . . . . .` | Store Zero ($00) to Absolute indexed by X (65C02) |
| **TAX** | `$AA` | Implied | 1 | 2 | `N . . . . . Z .` | Transfer A to X (`X = A`) |
| **TXA** | `$8A` | Implied | 1 | 2 | `N . . . . . Z .` | Transfer X to A (`A = X`) |
| **TAY** | `$A8` | Implied | 1 | 2 | `N . . . . . Z .` | Transfer A to Y (`Y = A`) |
| **TYA** | `$98` | Implied | 1 | 2 | `N . . . . . Z .` | Transfer Y to A (`A = Y`) |
| **TSX** | `$BA` | Implied | 1 | 2 | `N . . . . . Z .` | Transfer Stack Pointer to X (`X = SP & 0xFF`) |
| **TXS** | `$9A` | Implied | 1 | 2 | `. . . . . . . .` | Transfer X to Stack Pointer (`SP = 0x0100 \| X`) |

---

### 2. Arithmetic & Increments/Decrements

| Mnemonic | Opcode | Mode | Bytes | Cycles | Flags (N V - B D I Z C) | Description |
| :--- | :---: | :--- | :---: | :---: | :---: | :--- |
| **ADC** | `$69` | Immediate | 2 | 2 | `N V . . . . Z C` | Add with Carry (`A + M + C`) |
| | `$65` | Zero Page | 2 | 3 | `N V . . . . Z C` | Add memory from Zero Page with Carry |
| | `$75` | Zero Page, X | 2 | 4 | `N V . . . . Z C` | Add memory from Zero Page indexed by X |
| | `$6D` | Absolute | 3 | 4 | `N V . . . . Z C` | Add memory from Absolute address |
| | `$7D` | Absolute, X | 3 | 4* | `N V . . . . Z C` | Add memory from Absolute indexed by X (*+1 on page cross) |
| | `$79` | Absolute, Y | 3 | 4* | `N V . . . . Z C` | Add memory from Absolute indexed by Y (*+1 on page cross) |
| | `$61` | (Indirect, X) | 2 | 6 | `N V . . . . Z C` | Add memory from Indexed Indirect |
| | `$71` | (Indirect), Y | 2 | 5* | `N V . . . . Z C` | Add memory from Indirect Indexed (*+1 on page cross) |
| | `$72` | (Indirect) ZP | 2 | 5 | `N V . . . . Z C` | Add memory from Zero Page Indirect (65C02) |
| **SBC** | `$E9` | Immediate | 2 | 2 | `N V . . . . Z C` | Subtract with Carry (`A - M - ~C`) |
| | `$E5` | Zero Page | 2 | 3 | `N V . . . . Z C` | Subtract memory from Zero Page with Carry |
| | `$F5` | Zero Page, X | 2 | 4 | `N V . . . . Z C` | Subtract memory from Zero Page indexed by X |
| | `$ED` | Absolute | 3 | 4 | `N V . . . . Z C` | Subtract memory from Absolute address |
| | `$FD` | Absolute, X | 3 | 4* | `N V . . . . Z C` | Subtract memory from Absolute indexed by X (*+1 on page cross) |
| | `$F9` | Absolute, Y | 3 | 4* | `N V . . . . Z C` | Subtract memory from Absolute indexed by Y (*+1 on page cross) |
| | `$E1` | (Indirect, X) | 2 | 6 | `N V . . . . Z C` | Subtract memory from Indexed Indirect |
| | `$F1` | (Indirect), Y | 2 | 5* | `N V . . . . Z C` | Subtract memory from Indirect Indexed (*+1 on page cross) |
| | `$F2` | (Indirect) ZP | 2 | 5 | `N V . . . . Z C` | Subtract memory from Zero Page Indirect (65C02) |
| **INC** | `$1A` | Accumulator | 1 | 2 | `N . . . . . Z .` | Increment Accumulator (`A = A + 1`) (65C02) |
| | `$E6` | Zero Page | 2 | 5 | `N . . . . . Z .` | Increment Zero Page memory |
| | `$F6` | Zero Page, X | 2 | 6 | `N . . . . . Z .` | Increment Zero Page memory indexed by X |
| | `$EE` | Absolute | 3 | 6 | `N . . . . . Z .` | Increment Absolute memory |
| | `$FE` | Absolute, X | 3 | 6 | `N . . . . . Z .` | Increment Absolute memory indexed by X |
| **DEC** | `$3A` | Accumulator | 1 | 2 | `N . . . . . Z .` | Decrement Accumulator (`A = A - 1`) (65C02) |
| | `$C6` | Zero Page | 2 | 5 | `N . . . . . Z .` | Decrement Zero Page memory |
| | `$D6` | Zero Page, X | 2 | 6 | `N . . . . . Z .` | Decrement Zero Page memory indexed by X |
| | `$CE` | Absolute | 3 | 6 | `N . . . . . Z .` | Decrement Absolute memory |
| | `$DE` | Absolute, X | 3 | 6 | `N . . . . . Z .` | Decrement Absolute memory indexed by X |
| **INX** | `$E8` | Implied | 1 | 2 | `N . . . . . Z .` | Increment X Register (`X = X + 1`) |
| **DEX** | `$CA` | Implied | 1 | 2 | `N . . . . . Z .` | Decrement X Register (`X = X - 1`) |
| **INY** | `$C8` | Implied | 1 | 2 | `N . . . . . Z .` | Increment Y Register (`Y = Y + 1`) |
| **DEY** | `$88` | Implied | 1 | 2 | `N . . . . . Z .` | Decrement Y Register (`Y = Y - 1`) |

---

### 3. Logic, Comparisons, and Bit Operations

| Mnemonic | Opcode | Mode | Bytes | Cycles | Flags (N V - B D I Z C) | Description |
| :--- | :---: | :--- | :---: | :---: | :---: | :--- |
| **AND** | `$29` | Immediate | 2 | 2 | `N . . . . . Z .` | Logical AND Accumulator with constant |
| | `$25` | Zero Page | 2 | 3 | `N . . . . . Z .` | Logical AND Accumulator with Zero Page |
| | `$35` | Zero Page, X | 2 | 4 | `N . . . . . Z .` | Logical AND with Zero Page indexed by X |
| | `$2D` | Absolute | 3 | 4 | `N . . . . . Z .` | Logical AND with Absolute memory |
| | `$3D` | Absolute, X | 3 | 4* | `N . . . . . Z .` | Logical AND with Absolute indexed by X (*+1 on page cross) |
| | `$39` | Absolute, Y | 3 | 4* | `N . . . . . Z .` | Logical AND with Absolute indexed by Y (*+1 on page cross) |
| | `$21` | (Indirect, X) | 2 | 6 | `N . . . . . Z .` | Logical AND with Indexed Indirect |
| | `$31` | (Indirect), Y | 2 | 5* | `N . . . . . Z .` | Logical AND with Indirect Indexed (*+1 on page cross) |
| | `$32` | (Indirect) ZP | 2 | 5 | `N . . . . . Z .` | Logical AND with Zero Page Indirect (65C02) |
| **ORA** | `$09` | Immediate | 2 | 2 | `N . . . . . Z .` | Inclusive OR Accumulator with constant |
| | `$05` | Zero Page | 2 | 3 | `N . . . . . Z .` | Inclusive OR with Zero Page |
| | `$15` | Zero Page, X | 2 | 4 | `N . . . . . Z .` | Inclusive OR with Zero Page indexed by X |
| | `$0D` | Absolute | 3 | 4 | `N . . . . . Z .` | Inclusive OR with Absolute memory |
| | `$1D` | Absolute, X | 3 | 4* | `N . . . . . Z .` | Inclusive OR with Absolute indexed by X (*+1 on page cross) |
| | `$19` | Absolute, Y | 3 | 4* | `N . . . . . Z .` | Inclusive OR with Absolute indexed by Y (*+1 on page cross) |
| | `$01` | (Indirect, X) | 2 | 6 | `N . . . . . Z .` | Inclusive OR with Indexed Indirect |
| | `$11` | (Indirect), Y | 2 | 5* | `N . . . . . Z .` | Inclusive OR with Indirect Indexed (*+1 on page cross) |
| | `$12` | (Indirect) ZP | 2 | 5 | `N . . . . . Z .` | Inclusive OR with Zero Page Indirect (65C02) |
| **EOR** | `$49` | Immediate | 2 | 2 | `N . . . . . Z .` | Exclusive OR (XOR) with constant |
| | `$45` | Zero Page | 2 | 3 | `N . . . . . Z .` | Exclusive OR with Zero Page |
| | `$55` | Zero Page, X | 2 | 4 | `N . . . . . Z .` | Exclusive OR with Zero Page indexed by X |
| | `$4D` | Absolute | 3 | 4 | `N . . . . . Z .` | Exclusive OR with Absolute memory |
| | `$5D` | Absolute, X | 3 | 4* | `N . . . . . Z .` | Exclusive OR with Absolute indexed by X (*+1 on page cross) |
| | `$59` | Absolute, Y | 3 | 4* | `N . . . . . Z .` | Exclusive OR with Absolute indexed by Y (*+1 on page cross) |
| | `$41` | (Indirect, X) | 2 | 6 | `N . . . . . Z .` | Exclusive OR with Indexed Indirect |
| | `$51` | (Indirect), Y | 2 | 5* | `N . . . . . Z .` | Exclusive OR with Indirect Indexed (*+1 on page cross) |
| | `$52` | (Indirect) ZP | 2 | 5 | `N . . . . . Z .` | Exclusive OR with Zero Page Indirect (65C02) |
| **BIT** | `$89` | Immediate | 2 | 2 | `. . . . . . Z .` | Bit Test Immediate (65C02): Only updates Z flag (`A & M`) |
| | `$24` | Zero Page | 2 | 3 | `N V . . . . Z .` | Bit Test ZP: `Z = !(A & M)`, `N = M.7`, `V = M.6` |
| | `$34` | Zero Page, X | 2 | 4 | `N V . . . . Z .` | Bit Test ZP indexed by X (65C02) |
| | `$2C` | Absolute | 3 | 4 | `N V . . . . Z .` | Bit Test Absolute |
| | `$3C` | Absolute, X | 3 | 4* | `N V . . . . Z .` | Bit Test Absolute indexed by X (65C02) (*+1 on page cross) |
| **CMP** | `$C9` | Immediate | 2 | 2 | `N . . . . . Z C` | Compare Accumulator with constant (`A - M`) |
| | `$C5` | Zero Page | 2 | 3 | `N . . . . . Z C` | Compare Accumulator with Zero Page |
| | `$D5` | Zero Page, X | 2 | 4 | `N . . . . . Z C` | Compare Accumulator with Zero Page indexed by X |
| | `$CD` | Absolute | 3 | 4 | `N . . . . . Z C` | Compare Accumulator with Absolute address |
| | `$DD` | Absolute, X | 3 | 4* | `N . . . . . Z C` | Compare Accumulator with Absolute indexed by X (*+1 on page cross) |
| | `$D9` | Absolute, Y | 3 | 4* | `N . . . . . Z C` | Compare Accumulator with Absolute indexed by Y (*+1 on page cross) |
| | `$C1` | (Indirect, X) | 2 | 6 | `N . . . . . Z C` | Compare Accumulator with Indexed Indirect |
| | `$D1` | (Indirect), Y | 2 | 5* | `N . . . . . Z C` | Compare Accumulator with Indirect Indexed (*+1 on page cross) |
| | `$D2` | (Indirect) ZP | 2 | 5 | `N . . . . . Z C` | Compare Accumulator with Zero Page Indirect (65C02) |
| **CPX** | `$E0` | Immediate | 2 | 2 | `N . . . . . Z C` | Compare X Register with constant (`X - M`) |
| | `$E4` | Zero Page | 2 | 3 | `N . . . . . Z C` | Compare X Register with Zero Page |
| | `$EC` | Absolute | 3 | 4 | `N . . . . . Z C` | Compare X Register with Absolute address |
| **CPY** | `$C0` | Immediate | 2 | 2 | `N . . . . . Z C` | Compare Y Register with constant (`Y - M`) |
| | `$C4` | Zero Page | 2 | 3 | `N . . . . . Z C` | Compare Y Register with Zero Page |
| | `$CC` | Absolute | 3 | 4 | `N . . . . . Z C` | Compare Y Register with Absolute address |
| **TRB** | `$14` | Zero Page | 2 | 5 | `. . . . . . Z .` | Test and Reset Bits in Zero Page (65C02) |
| | `$1C` | Absolute | 3 | 6 | `. . . . . . Z .` | Test and Reset Bits in Absolute memory (65C02) |
| **TSB** | `$04` | Zero Page | 2 | 5 | `. . . . . . Z .` | Test and Set Bits in Zero Page (65C02) |
| | `$0C` | Absolute | 3 | 6 | `. . . . . . Z .` | Test and Set Bits in Absolute memory (65C02) |
| **RMB0–7**| `$07`+`n*$10`| Zero Page | 2 | 5 | `. . . . . . . .` | Reset Memory Bit *n* (0–7) in Zero Page (65C02) |
| **SMB0–7**| `$87`+`n*$10`| Zero Page | 2 | 5 | `. . . . . . . .` | Set Memory Bit *n* (0–7) in Zero Page (65C02) |

---

### 4. Shifts and Rotates

| Mnemonic | Opcode | Mode | Bytes | Cycles | Flags (N V - B D I Z C) | Description |
| :--- | :---: | :--- | :---: | :---: | :---: | :--- |
| **ASL** | `$0A` | Accumulator | 1 | 2 | `N . . . . . Z C` | Arithmetic Shift Left Accumulator |
| | `$06` | Zero Page | 2 | 5 | `N . . . . . Z C` | Arithmetic Shift Left Zero Page memory |
| | `$16` | Zero Page, X | 2 | 6 | `N . . . . . Z C` | Arithmetic Shift Left ZP indexed by X |
| | `$0E` | Absolute | 3 | 6 | `N . . . . . Z C` | Arithmetic Shift Left Absolute memory |
| | `$1E` | Absolute, X | 3 | 6 | `N . . . . . Z C` | Arithmetic Shift Left Absolute indexed by X |
| **LSR** | `$4A` | Accumulator | 1 | 2 | `0 . . . . . Z C` | Logical Shift Right Accumulator (Bit 7 becomes 0) |
| | `$46` | Zero Page | 2 | 5 | `0 . . . . . Z C` | Logical Shift Right Zero Page memory |
| | `$56` | Zero Page, X | 2 | 6 | `0 . . . . . Z C` | Logical Shift Right ZP indexed by X |
| | `$4E` | Absolute | 3 | 6 | `0 . . . . . Z C` | Logical Shift Right Absolute memory |
| | `$5E` | Absolute, X | 3 | 6 | `0 . . . . . Z C` | Logical Shift Right Absolute indexed by X |
| **ROL** | `$2A` | Accumulator | 1 | 2 | `N . . . . . Z C` | Rotate Left through Carry Accumulator |
| | `$26` | Zero Page | 2 | 5 | `N . . . . . Z C` | Rotate Left through Carry Zero Page memory |
| | `$36` | Zero Page, X | 2 | 6 | `N . . . . . Z C` | Rotate Left through Carry ZP indexed by X |
| | `$2E` | Absolute | 3 | 6 | `N . . . . . Z C` | Rotate Left through Carry Absolute memory |
| | `$3E` | Absolute, X | 3 | 6 | `N . . . . . Z C` | Rotate Left through Carry Absolute indexed by X |
| **ROR** | `$6A` | Accumulator | 1 | 2 | `N . . . . . Z C` | Rotate Right through Carry Accumulator |
| | `$66` | Zero Page | 2 | 5 | `N . . . . . Z C` | Rotate Right through Carry Zero Page memory |
| | `$76` | Zero Page, X | 2 | 6 | `N . . . . . Z C` | Rotate Right through Carry ZP indexed by X |
| | `$6E` | Absolute | 3 | 6 | `N . . . . . Z C` | Rotate Right through Carry Absolute memory |
| | `$7E` | Absolute, X | 3 | 6 | `N . . . . . Z C` | Rotate Right through Carry Absolute indexed by X |

---

### 5. Branches, Jumps, and Subroutines

| Mnemonic | Opcode | Mode | Bytes | Cycles | Flags (N V - B D I Z C) | Description |
| :--- | :---: | :--- | :---: | :---: | :---: | :--- |
| **BRA** | `$80` | Relative | 2 | 3* | `. . . . . . . .` | Branch Always (65C02) (*+1 on page cross) |
| **BCC** | `$90` | Relative | 2 | 2** | `. . . . . . . .` | Branch if Carry Clear (`C == 0`) |
| **BCS** | `$B0` | Relative | 2 | 2** | `. . . . . . . .` | Branch if Carry Set (`C == 1`) |
| **BEQ** | `$F0` | Relative | 2 | 2** | `. . . . . . . .` | Branch if Equal / Zero Set (`Z == 1`) |
| **BNE** | `$D0` | Relative | 2 | 2** | `. . . . . . . .` | Branch if Not Equal / Zero Clear (`Z == 0`) |
| **BMI** | `$30` | Relative | 2 | 2** | `. . . . . . . .` | Branch if Minus / Negative Set (`N == 1`) |
| **BPL** | `$10` | Relative | 2 | 2** | `. . . . . . . .` | Branch if Plus / Negative Clear (`N == 0`) |
| **BVC** | `$50` | Relative | 2 | 2** | `. . . . . . . .` | Branch if Overflow Clear (`V == 0`) |
| **BVS** | `$70` | Relative | 2 | 2** | `. . . . . . . .` | Branch if Overflow Set (`V == 1`) |
| **BBR0–7**| `$0F`+`n*$10`| ZP Relative | 3 | 5*** | `. . . . . . . .` | Branch if Bit *n* Reset in ZP (65C02) |
| **BBS0–7**| `$8F`+`n*$10`| ZP Relative | 3 | 5*** | `. . . . . . . .` | Branch if Bit *n* Set in ZP (65C02) |
| **JMP** | `$4C` | Absolute | 3 | 3 | `. . . . . . . .` | Jump to Absolute address (`PC = addr`) |
| | `$6C` | Indirect | 3 | 6 | `. . . . . . . .` | Jump Indirect (`PC = mem[addr]`) |
| | `$7C` | Absolute, X | 3 | 6 | `. . . . . . . .` | Jump Absolute Indexed Indirect (`PC = mem[addr+X]`) (65C02) |
| **JSR** | `$20` | Absolute | 3 | 6 | `. . . . . . . .` | Jump to Subroutine (pushes `PC - 1` to stack) |
| **RTS** | `$60` | Implied | 1 | 6 | `. . . . . . . .` | Return from Subroutine (pops `PC`, increments) |
| **RTI** | `$40` | Implied | 1 | 6 | `Restores All` | Return from Interrupt (pops Status, then `PC`) |

> `**` **Conditional Branch Timing:**
> - Branch not taken: **2 cycles**
> - Branch taken, target on same page: **3 cycles**
> - Branch taken, target on different page: **4 cycles**
>
> `***` **BBR/BBS Timing:**
> - Bit condition not met: **5 cycles**
> - Bit condition met, branch taken: **6 cycles**
> - Bit condition met, branch crosses page: **7 cycles**

---

### 6. Stack Operations

| Mnemonic | Opcode | Mode | Bytes | Cycles | Flags (N V - B D I Z C) | Description |
| :--- | :---: | :--- | :---: | :---: | :---: | :--- |
| **PHA** | `$48` | Implied | 1 | 3 | `. . . . . . . .` | Push Accumulator to stack (`mem[SP--] = A`) |
| **PLA** | `$68` | Implied | 1 | 4 | `N . . . . . Z .` | Pull Accumulator from stack (`A = mem[++SP]`) |
| **PHP** | `$08` | Implied | 1 | 3 | `. . . . . . . .` | Push Status Register to stack (with `B=1` and `Bit 5=1`) |
| **PLP** | `$28` | Implied | 1 | 4 | `Restores All` | Pull Status Register from stack |
| **PHX** | `$DA` | Implied | 1 | 3 | `. . . . . . . .` | Push X Register to stack (`mem[SP--] = X`) (65C02) |
| **PLX** | `$FA` | Implied | 1 | 4 | `N . . . . . Z .` | Pull X Register from stack (`X = mem[++SP]`) (65C02) |
| **PHY** | `$5A` | Implied | 1 | 3 | `. . . . . . . .` | Push Y Register to stack (`mem[SP--] = Y`) (65C02) |
| **PLY** | `$7A` | Implied | 1 | 4 | `N . . . . . Z .` | Pull Y Register from stack (`Y = mem[++SP]`) (65C02) |

---

### 7. Status Flags & System Control

| Mnemonic | Opcode | Mode | Bytes | Cycles | Flags (N V - B D I Z C) | Description |
| :--- | :---: | :--- | :---: | :---: | :---: | :--- |
| **CLC** | `$18` | Implied | 1 | 2 | `. . . . . . . 0` | Clear Carry Flag (`C = 0`) |
| **SEC** | `$38` | Implied | 1 | 2 | `. . . . . . . 1` | Set Carry Flag (`C = 1`) |
| **CLI** | `$58` | Implied | 1 | 2 | `. . . . . 0 . .` | Clear Interrupt Disable (`I = 0`, enables IRQs) |
| **SEI** | `$78` | Implied | 1 | 2 | `. . . . . 1 . .` | Set Interrupt Disable (`I = 1`, masks IRQs) |
| **CLD** | `$D8` | Implied | 1 | 2 | `. . . . 0 . . .` | Clear Decimal Mode (`D = 0`) |
| **SED** | `$F8` | Implied | 1 | 2 | `. . . . 1 . . .` | Set Decimal Mode (`D = 1`) |
| **CLV** | `$B8` | Implied | 1 | 2 | `. 0 . . . . . .` | Clear Overflow Flag (`V = 0`) |
| **NOP** | `$EA` | Implied | 1 | 2 | `. . . . . . . .` | No Operation |
| **BRK** | `$00` | Implied | 1 (2*) | 7 | `. . . . 0 1 . .` | Software Break: pushes `PC+1`, Status (`B=1`), jumps to `$FFFE` |
| **WAI** | `$CB` | Implied | 1 | 3 | `. . . . . . . .` | Wait for Interrupt: halts CPU until IRQ/NMI (65C02) |
| **STP** | `$DB` | Implied | 1 | 3 | `. . . . . . . .` | Stop Processor: permanently halts core until Reset (65C02) |

> `*` **BRK Instruction Length:** While `BRK` is technically a 1-byte opcode (`$00`), the 6502/65C02 architecture advances `PC` by 2 before pushing the return address to the stack. This reserves the byte following `BRK` as an optional software interrupt signature/break code.

#### 65C02 Defined Multi-Byte NOP Extensions

Unlike the NMOS 6502 where undefined opcodes frequently locked up the processor, the WDC 65C02 architecture defines reserved opcodes as standardized multi-byte and multi-cycle `NOP` instructions:

| Mnemonic | Opcode(s) | Addressing Mode | Bytes | Cycles | Operation |
| :--- | :--- | :--- | :---: | :---: | :--- |
| **NOP** | `$EA`, `$03`, `$0B`, `$13`, `$1B`, `$23`, `$2B`, `$33`, `$3B`, `$43`, `$4B`, `$53`, `$5B`, `$63`, `$6B`, `$73`, `$7B`, `$83`, `$8B`, `$93`, `$9B`, `$A3`, `$AB`, `$B3`, `$BB`, `$C3`, `$D3`, `$E3`, `$EB`, `$F3`, `$FB` | Implied | 1 | 2 | Standard 1-byte No Operation |
| **NOP #imm** | `$02`, `$22`, `$42`, `$62`, `$82`, `$C2`, `$E2` | Immediate | 2 | 2 | 2-byte NOP, fetches and discards 8-bit immediate byte |
| **NOP zp** | `$44` | Zero Page | 2 | 3 | 2-byte NOP, reads and discards Zero Page operand |
| **NOP zp,X** | `$54`, `$D4`, `$F4` | Zero Page, X | 2 | 4 | 2-byte NOP, reads and discards indexed Zero Page operand |
| **NOP abs** | `$5C` | Absolute | 3 | 4 | 3-byte NOP, reads and discards 16-bit address |
| **NOP abs,X** | `$DC`, `$FC` | Absolute, X | 3 | 4 | 3-byte NOP, reads and discards indexed 16-bit address |

---

## Opcode Implementation Architecture

Each opcode in 65c02-SIM is encapsulated in a dedicated header/implementation pair inside [`src/Hardware/CPU/Instructions/`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Hardware/CPU/Instructions/).

For example, [`LoadStore.h`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Hardware/CPU/Instructions/LoadStore.h) defines `STZ`:

```cpp
class STZ {
public:
    template <bool Debug>
    static void ExecuteZP(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void ExecuteZPX(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void ExecuteABS(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void ExecuteABSX(CPU& cpu, Bus& bus);
};

template <bool Debug>
inline void STZ::ExecuteZP(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte<Debug>(bus);
    bus.Write<Debug>(ZP_Dir, 0);
}
```

- **Inlining & LTO**: Keeping the executor functions `inline` and templated on `<bool Debug>` allows Link-Time Optimization (LTO) to eliminate function preamble/epilogue overhead.
- **Trace Hooks**: Debug branches (like `cpu.TraceJSR<Debug>(Dir, RetDir)`) are evaluated at compile time via `if constexpr (Debug)`. In production runs, zero branching instructions or profiler checks are emitted in the hot loop.
