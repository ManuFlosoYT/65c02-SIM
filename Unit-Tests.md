# Unit Testing Suite

The **65c02-SIM** unit testing infrastructure provides comprehensive test coverage across CPU instructions, peripheral devices, operating system subsystems, file systems, debugger utilities, and scripting interpreters.

---

## Table of Contents

- [Testing Architecture](#testing-architecture)
  - [Google Test Framework](#google-test-framework)
  - [Unity Build Configuration](#unity-build-configuration)
  - [Precompiled Headers (PCH)](#precompiled-headers-pch)
- [Comprehensive Test Coverage](#comprehensive-test-coverage)
  - [1. 65C02 Instruction Test Suite](#1-65c02-instruction-test-suite)
  - [2. Hardware & Peripherals](#2-hardware--peripherals)
  - [3. System & Core Subsystems](#3-system--core-subsystems)
  - [4. microDOS Operating System & Shell](#4-microdos-operating-system--shell)
  - [5. Custom Scripting Engine](#5-custom-scripting-engine)
- [Running Unit Tests](#running-unit-tests)
  - [Automated Execution via `build.sh`](#automated-execution-via-buildsh)
  - [Direct Binary Execution](#direct-binary-execution)
  - [CTest Integration](#ctest-integration)
  - [Filter Patterns (`--gtest_filter`)](#filter-patterns---gtest_filter)
  - [Cross-Compiled Execution (Wine)](#cross-compiled-execution-wine)
- [Writing New Tests](#writing-new-tests)
  - [Fixture Setup Pattern](#fixture-setup-pattern)
  - [Instruction Execution & STP/JAM Technique](#instruction-execution--stpjam-technique)
  - [Adding Tests to the CMake Harness](#adding-tests-to-the-cmake-harness)

---

## Testing Architecture

### Google Test Framework

The suite is built on **Google Test** (`GTest::gtest_main`). Tests are registered automatically via CMake's `GoogleTest` module using `gtest_discover_tests(unit_tests ...)` with a default execution timeout of 30 seconds per test.

### Unity Build Configuration

Given that the test suite includes hundreds of individual test cases across CPU opcodes and hardware subsystems, `cmake/UnitTests.cmake` leverages CMake's **Unity Build** feature to reduce compile and link times:

```cmake
set_target_properties(unit_tests PROPERTIES
    UNITY_BUILD ON
    UNITY_BUILD_BATCH_SIZE 16
    UNITY_BUILD_MODE BATCH
)
```

- **Batch Size 16:** CMake automatically combines up to 16 test translation units into single amalgamation source files, slashing compiler process spawn overhead and duplicate template instantiation.
- **Selective Isolation (`SKIP_UNITY_BUILD_INCLUSION`):** Heavyweight integration tests, tests defining conflicting mock classes or anonymous namespaces, and multi-threaded synchronization tests are explicitly exempted from unity batching:
  - `System_Savestate_Test.cpp`
  - `System_WAI_Threading_Test.cpp`
  - `System_Cartridge_Test.cpp`
  - `RasterBeam_Test.cpp`
  - `LogicAnalyzer_Test.cpp`
  - `Interrupts_Test.cpp`
  - `MicroDosGPU_Test.cpp`
  - `MicroDosDesktop_Test.cpp`
  - `Font5x5_Test.cpp`
  - `Font3x5_Test.cpp`
  - `Shell_Pipeline_Test.cpp`
  - `Shell_History_Completion_Test.cpp`
  - `MicroDos_FSCommands_Test.cpp`
  - `Batch_Script_Test.cpp`
  - Audio/MIDI helpers (`MIDIFile.cpp`, `MIDISoundBank.cpp`, `MIDISoundBankLoader.cpp`, `MIDIVoiceAllocator.cpp`)

### Precompiled Headers (PCH)

To further maximize compile performance, `cmake/UnitTests.cmake` precompiles foundational headers shared across test units:

```cmake
target_precompile_headers(unit_tests PRIVATE
    <gtest/gtest.h>
    <cstdint>
    <vector>
    <string>
    <memory>
    <algorithm>
    "${CMAKE_SOURCE_DIR}/src/Hardware/CPU/CPU.h"
    "${CMAKE_SOURCE_DIR}/src/Hardware/Core/Bus.h"
    "${CMAKE_SOURCE_DIR}/src/Hardware/Memory/RAM.h"
    "${CMAKE_SOURCE_DIR}/src/Hardware/CPU/Instructions/InstructionSet.h"
)
```

Combined with `ccache`, incremental test recompilations execute in seconds.

---

## Comprehensive Test Coverage

### 1. 65C02 Instruction Test Suite

The emulator includes an exhaustive validation suite covering every opcode, addressing mode, cycle count, and flag modification of the **WDC 65C02 CMOS** specification:

#### Instruction Matrix

- **Load & Store Registers:**
  - `LDA/` — Load Accumulator with all 8 addressing modes: Immediate (`INS_LDA_IM`), Zero Page (`INS_LDA_ZP`), Zero Page X (`INS_LDA_ZPX`), Absolute (`INS_LDA_ABS`), Absolute X (`INS_LDA_ABSX`), Absolute Y (`INS_LDA_ABSY`), Indexed Indirect (`INS_LDA_INDX`), Indirect Indexed (`INS_LDA_INDY`), and 65C02 Zero Page Indirect (`INS_LDA_IND_ZP`).
  - `LDX/` — Load X Register: Immediate (`INS_LDX_IM`), Zero Page (`INS_LDX_ZP`), Zero Page Y (`INS_LDX_ZPY`), Absolute (`INS_LDX_ABS`), Absolute Y (`INS_LDX_ABSY`).
  - `LDY/` — Load Y Register: Immediate (`INS_LDY_IM`), Zero Page (`INS_LDY_ZP`), Zero Page X (`INS_LDY_ZPX`), Absolute (`INS_LDY_ABS`), Absolute X (`INS_LDY_ABSX`).
  - `STA/` — Store Accumulator to memory: Zero Page (`INS_STA_ZP`), Zero Page X (`INS_STA_ZPX`), Absolute (`INS_STA_ABS`), Absolute X (`INS_STA_ABSX`), Absolute Y (`INS_STA_ABSY`), Indexed Indirect (`INS_STA_INDX`), Indirect Indexed (`INS_STA_INDY`), and 65C02 Zero Page Indirect (`INS_STA_IND_ZP`).
  - `STX/` — Store X Register: Zero Page (`INS_STX_ZP`), Zero Page Y (`INS_STX_ZPY`), Absolute (`INS_STX_ABS`).
  - `STY/` — Store Y Register: Zero Page (`INS_STY_ZP`), Zero Page X (`INS_STY_ZPX`), Absolute (`INS_STY_ABS`).
  - `STZ/` — Store Zero (65C02 extension: Direct zeroing of Zero Page, Absolute, and indexed memory without modifying $A$).
- **Arithmetic & Logic:**
  - `ADC/` — Add with Carry (Binary and Decimal BCD modes, overflow $V$ calculations, all 8 addressing modes).
  - `SBC/` — Subtract with Carry (Borrow inversion, BCD subtraction, sign flag behaviors).
  - `AND/`, `ORA/`, `EOR/` — Bitwise logical operations across immediate, direct, indexed, and indirect modes.
  - `CMP/`, `CPX/`, `CPY/` — Register magnitude comparisons against memory and constants.
  - `BIT/` — Bit test (validates both classic absolute/ZP modes modifying $N, V, Z$, and 65C02 immediate mode modifying only $Z$).
- **Shifts & Rotates:**
  - `ASL/`, `LSR/` — Arithmetic shift left and logical shift right (Accumulator and memory modes, carry preservation).
  - `ROL/`, `ROR/` — 9-bit rotations through the Carry flag.
- **Increment, Decrement & Zeroing:**
  - `INC/`, `DEC/` — Memory increment and decrement, including 65C02 `INC A` and `DEC A` accumulator forms.
  - `INX.cpp`, `INY.cpp`, `DEX.cpp`, `DEY.cpp` — Register indexing increments/decrements.
- **Bit Manipulation (Rockwell / WDC Extensions):**
  - `RMB.cpp` (`RMB0`–`RMB7`) — Reset memory bit in zero page.
  - `SMB.cpp` (`SMB0`–`SMB7`) — Set memory bit in zero page.
  - `BBR.cpp` (`BBR0`–`BBR7`) — Branch if memory bit is reset (0).
  - `BBS.cpp` (`BBS0`–`BBS7`) — Branch if memory bit is set (1).
  - `TRB/` — Test and Reset Bits against Accumulator.
  - `TSB/` — Test and Set Bits against Accumulator.
- **Branching & Control Flow:**
  - Conditional branches: `BCC.cpp`, `BCS.cpp`, `BEQ.cpp`, `BMI.cpp`, `BNE.cpp`, `BPL.cpp`, `BVC.cpp`, `BVS.cpp`.
  - Unconditional branch: `BRA.cpp` (65C02 relative jump).
  - Subroutine calls: `JSR.cpp`, `RTS.cpp`.
  - Jumps: `JMP/` (Absolute, Absolute Indirect `JMP (abs)` including 65C02 page-boundary bug fix, and Absolute Indexed Indirect `JMP (abs,X)`).
  - System control: `BRK.cpp` (Software break vectoring), `RTI.cpp` (Return from interrupt), `STP.cpp` (Processor halt), `NOP.cpp` (No Operation).
- **Stack Operations & Transfers:**
  - `PHA.cpp`, `PLA.cpp`, `PHP.cpp`, `PLP.cpp` — Accumulator and Status register push/pull (validating break flag bit-4 and bit-5 rules).
  - `PHX.cpp`, `PHY.cpp`, `PLX.cpp`, `PLY.cpp` — 65C02 Index register stack push/pull.
  - `TAX.cpp`, `TAY.cpp`, `TSX.cpp`, `TXA.cpp`, `TXS.cpp`, `TYA.cpp` — Register transfers and flag side-effects.
- **Flags:**
  - `CLC.cpp`, `SEC.cpp` — Clear / Set Carry.
  - `CLD.cpp`, `SED.cpp` — Clear / Set Decimal mode.
  - `CLI.cpp`, `SEI.cpp` — Clear / Set Interrupt Disable.
  - `CLV.cpp` — Clear Overflow.

#### Multi-Instruction Integration Programs

Located in `src/UnitTests/Programs/`:
- `Fibonacci.cpp` — Computes 8-bit and 16-bit Fibonacci sequences in memory.
- `Multiplication.cpp` — Verifies iterative shift-and-add 16-bit multiplication routines.
- `MemoryBlock.cpp` — Verifies block copy and buffer fill routines using indexed indirect pointers.

---

### 2. Hardware & Peripherals

- **VIA 6522 Versatile Interface Adapter (`VIA_Test.cpp`):**
  - Timer 1 (T1) and Timer 2 (T2) countdown, one-shot vs. continuous free-running modes.
  - PB7 square-wave output toggle frequency.
  - Data Direction Registers (DDRA, DDRB) and input/output latching.
  - Shift register modes and interrupt flag generation.
- **ACIA 6551 Serial Interface (`ACIA_Test.cpp`):**
  - Baud rate clock divider registers.
  - Transmitter Data Register (TDR) and Receiver Data Register (RDR) queues.
  - Status Register bit flags (DSR, CTS, TDRE, RDRF, Overrun, Parity, Framing).
  - Command and Control register configuration.
- **SID 6581/8580 Sound Generator:**
  - `SIDResampler_Test.cpp` — Validates band-limited sinc resampling, anti-aliasing filter response, and sample rate conversion from SID clock (1.0 MHz) to audio host rates (48 kHz).
  - `SID_MuteSolo_Test.cpp` — Tests channel mute and solo matrices across the 3 independent voice oscillators, filter bypass routing, and ADSR envelope generation.
- **GPU & Video Architecture:**
  - `GPU_Test.cpp` — Validates 7.5 KB VRAM address decoding, 64-color palette translation, and scanline rendering buffers.
  - `RasterBeam_Test.cpp` — Tracks horizontal and vertical beam counter coordinates, H-Blank and V-Blank interrupt generation.
  - `MicroDosGPU_Test.cpp` — Validates high-resolution text mode (80x25 characters) and graphical blitting routines.
- **ESP8266 Virtual Wi-Fi Module (`ESP8266_Test.cpp`):**
  - AT command parser (`AT`, `AT+CWMODE`, `AT+CWJAP`, `AT+CIPSTART`, `AT+CIPSEND`, `AT+CIPCLOSE`).
  - Ring buffer transmission, byte flow control, and asynchronous socket mocking.
- **ANSI VT100 Terminal (`Console_Test.cpp`):**
  - Parses ANSI escape sequences (cursor positioning, color styling, screen clearing).
  - Terminal buffer wrap-around, history scrolling, and text output formatting.
- **LCD Display (`LCD_Test.cpp`):**
  - HD44780 controller command decoding (4-bit and 8-bit modes).
  - Character generation RAM (CGRAM) and display data RAM (DDRAM) address mapping.
- **Interrupt Controller (`Interrupts_Test.cpp`):**
  - Maskable Interrupt Request (IRQ) pin assertions and priority masking (`cpu.I`).
  - Non-Maskable Interrupt (NMI) negative-edge detection and priority preempting.
  - Hardware vector fetches (`$FFFA/$FFFB` for NMI, `$FFFE/$FFFF` for IRQ/BRK).

---

### 3. System & Core Subsystems

- **Cartridge System (`System_Cartridge_Test.cpp`):**
  - Parsing and extraction of `.65c` ZIP cartridge containers.
  - Schema validation for `manifest.json` (memory models, custom hardware flags, entry points).
  - Loading binary ROM, VRAM, and RAM segments into the emulator address space.
- **Save-State Serialization & Rollback (`System_Savestate_Test.cpp`):**
  - Full CPU, register, bus, memory, and peripheral serialization into binary snapshots (`SIM65C02SST`).
  - State compression via Miniz, SHA-256 integrity checksum verification, and atomic rollback on corrupt data.
- **WAI Instruction & Multithreading (`System_WAI_Threading_Test.cpp`):**
  - Validates the 65C02 `WAI` (Wait for Interrupt) instruction low-power sleep state.
  - Multithreaded synchronization: Verifies that worker threads wake up the emulation core without race conditions or lost cycles upon IRQ delivery.
- **VirtualFAT16 Engine (`System_VirtualFAT_Test.cpp`):**
  - Dynamic in-memory FAT16 file system synthesis for the virtual SD Card.
  - Partition table generation, cluster allocation chaining, root directory table parsing, and sector synchronization between host files and guest 6502 code.
- **Memory Tools & Analysis:**
  - `MemoryScanner_Test.cpp` — Exact value, relative increase/decrease, and changed-value memory searches across 64 KB RAM.
  - `Heatmap_Test.cpp` — Read and write access frequency tracking, cycle decay curves, and hot-spot detection.
  - `CDL_Disassembler_Test.cpp` — Code / Data Logging (CDL) analysis, classifying memory bytes into executable code, data tables, or indirect vectors.
  - `LogicAnalyzer_Test.cpp` — Digital 8-channel waveform capture tracking `PHI2`, `R/W`, `SYNC`, `IRQ`, `NMI`, and address bus activity.
  - `SubroutineProfiler_Test.cpp` — Function call-stack tracking, cumulative clock cycle accounting, invocation counts, and recursive function depth.
- **Audio Compilers & Tools:**
  - `MIDI_Pipeline_Test.cpp` — Parses Standard MIDI Files (.mid), allocates polyphonic voices, and validates conversion into assembly sound arrays for SID playback.
  - `NSFRunner_Test.cpp` — Header validation and execution loop for NES Sound Format audio files.
- **Modular Hardware Architecture (`Core_Hardware_Modular_Test.cpp`):**
  - `RasterTimingGenerator` — Pixel coordinates, horizontal/vertical blanking intervals, drawing wait cycles, and clock cycle skipping.
  - `MemoryPatternInitializer` — Deterministic zero-clearing and realistic pseudo-random uninitialized power-on memory patterns.
  - `SPSCQueue` — Lock-free Single-Producer Single-Consumer ring buffer queue capacity, push/pop semantics, and thread-safe fifo operations.
  - `IMemoryBlockDevice` — Unified polymorphic memory block interface validating RAM and ROM read-only constraints and raw buffer access.
- **Debugger Expression & Symbol Parsing:**
  - `BreakpointExpressionTest.cpp` — Complex conditional breakpoint expression evaluations (e.g. `A == 0x42 && (X > 5 || PC == 0xC000)`).
  - `SymbolTable_Test.cpp` — Parses DWARF debug info and CA65 `.dbg` symbol maps into memory addresses and source line numbers.
  - `IDE_BlankLines_Test.cpp` — Ensures editor line numbering preserves source-to-assembly synchronization during blank line edits.

---

### 4. microDOS Operating System & Shell

Dedicated integration tests verifying the native operating system running on 65c02-SIM:

- `MicroDosGPU_Test.cpp` — Text-mode rendering and 80x25 character grid formatting.
- `Font5x5_Test.cpp` & `Font3x5_Test.cpp` — Variable-pitch micro-font glyph decoding, bit-shift render blitting, and boundary clipping.
- `MicroDosDesktop_Test.cpp` — Graphical desktop window manager, window dragging, focus transitions, and clipping rect calculations.
- `Shell_Pipeline_Test.cpp` — Command piping (`cmd1 | cmd2`), input/output redirection (`>`, `>>`, `<`), and standard stream buffers.
- `Shell_History_Completion_Test.cpp` — Command history recall (Up/Down arrows), circular buffer storage, and Tab key path/binary auto-completion.
- `MicroDos_FSCommands_Test.cpp` — Filesystem utilities: `DIR`, `TYPE`, `COPY`, `DEL`, `MKDIR`, `CD`, `REN`, and file attribute management.
- `Batch_Script_Test.cpp` — Batch script interpreter (`.BAT`), script line execution, labels, `GOTO`, `IF` conditions, `CALL` sub-scripts, and positional parameter substitution (`%1`–`%9`).

---

### 5. Custom Scripting Engine

Tests located in `src/UnitTests/Scripting/` validating the built-in scripting language:

- `LexerTest.cpp` — Tokenization of keywords, decimal/hexadecimal/binary numerical literals, operators, strings, and comments.
- `ParserTest.cpp` — Abstract Syntax Tree (AST) construction, operator precedence rules, branch statements, loops, and function declarations.
- `InterpreterTest.cpp` — Scope stack, variable bindings, arithmetic expressions, function invocations, and emulator API bindings (`emu.step()`, `emu.read_mem()`, `emu.write_mem()`).

---

## Running Unit Tests

### Automated Execution via `build.sh`

The test suite is compiled and run automatically whenever you build the project using `./build.sh`:

```bash
# Builds project and executes all unit tests
./build.sh

# Debug build and execute tests
./build.sh --debug
```

### Direct Binary Execution

After compilation, the `unit_tests` executable is located in the target build directory:

```bash
# Run tests directly from Release build
./build/release/unit_tests

# Run tests directly from Debug build
./build/debug/unit_tests
```

### CTest Integration

You can invoke CMake's test driver directly across multiple parallel cores:

```bash
ctest --test-dir build/release -j $(nproc) --output-on-failure
```

### Filter Patterns (`--gtest_filter`)

Google Test supports wildcard filter patterns to run specific tests or skip slow integration tests:

```bash
# Run only ADC instruction tests
./build/release/unit_tests --gtest_filter="ADC_Test.*"

# Run all microDOS and Shell tests
./build/release/unit_tests --gtest_filter="MicroDos*:Shell*:Batch*"

# Run all tests EXCEPT slow stress tests
./build/release/unit_tests --gtest_filter="-*Stress*:*Benchmark*"

# Run only save-state and cartridge tests
./build/release/unit_tests --gtest_filter="System_Savestate*:System_Cartridge*"
```

### Cross-Compiled Execution (Wine)

When cross-compiling for Windows on Linux (`./build.sh --target win`), the script automatically detects `wine` and runs the Windows test executable:

```bash
wine ./build_win/release/unit_tests.exe
```

---

## Writing New Tests

### Fixture Setup Pattern

New tests should be written in modern C++23, inheriting from `::testing::Test`. Instantiate and register a `RAM` block device to the `Bus` within `SetUp()`, then invoke `cpu.Reset()`:

```cpp
#include <gtest/gtest.h>
#include "Hardware/CPU/CPU.h"
#include "Hardware/CPU/Instructions/InstructionSet.h"
#include "Hardware/Core/Bus.h"
#include "Hardware/Memory/RAM.h"

namespace Hardware {

class Example_Test : public ::testing::Test {
protected:
    void SetUp() override {
        // Map 64 KB RAM across entire address space
        bus.RegisterDevice(0x0000, 0xFFFF, &ram);
        cpu.Reset();
    }

    Bus bus;
    RAM ram{0x10000};
    CPU cpu;
};

TEST_F(Example_Test, ImmediateAddressingLoadsAccumulator) {
    // 1. Configure the 6502 reset vector ($FFFC/$FFFD) to entry address $4000
    bus.WriteDirect(0xFFFC, 0x00);
    bus.WriteDirect(0xFFFD, 0x40);

    // 2. Assemble instruction sequence: LDA #$42 ; STP
    bus.Write(0x4000, INS_LDA_IM);
    bus.Write(0x4001, 0x42);
    bus.Write(0x4002, INS_STP);

    // 3. Execute instruction sequence until STP halts execution
    cpu.Execute(bus);

    // 4. Assert register and flag states
    EXPECT_EQ(cpu.PC, 0x4003);
    EXPECT_EQ(cpu.A, 0x42);
    EXPECT_FALSE(cpu.Z);
    EXPECT_FALSE(cpu.N);
}

} // namespace Hardware
```

### Instruction Execution & STP/JAM Technique

Tests simulate individual instructions by writing the opcode to memory, followed immediately by an `INS_STP` (Stop / Jam, opcode `$DB`) instruction:
- Calling `cpu.Execute(bus)` triggers `cpu.EnsureInit(bus)`, which reads the reset vector at `$FFFC`/`$FFFD` to initialize `cpu.PC`.
- The CPU decodes and executes the target instruction, advancing `cpu.PC`.
- When the CPU encounters `INS_STP`, `Dispatch` returns `1` (STOP/JAM), terminating `cpu.Execute` cleanly without entering an infinite loop.
- The test suite then verifies register values, arithmetic flags (`cpu.Z`, `cpu.N`, `cpu.C`, `cpu.V`), cycle counters, and memory locations.

### Adding Tests to the CMake Harness

1. Place your `.cpp` test file in `src/UnitTests/` or an appropriate subfolder.
2. CMake automatically discovers new `.cpp` files via `file(GLOB_RECURSE TEST_SOURCES "src/UnitTests/*.cpp")`.
3. If your test file contains multi-threaded code, custom mocks, or symbols that conflict with unity batching, add the file path to `set_source_files_properties(... PROPERTIES SKIP_UNITY_BUILD_INCLUSION ON)` in `cmake/UnitTests.cmake`.
4. Re-run `./build.sh` to compile and execute your new tests.
