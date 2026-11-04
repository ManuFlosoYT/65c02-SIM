# Embedded Scripting Language & Test Automation

**Source Directory:** [`src/Scripting/`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Scripting/)  
**Technologies:** Custom AST-Interpreter Pipeline · Modern C++23 · Zero External Language Dependencies

---

## 1. Overview

**65c02-SIM** embeds a proprietary, lightweight scripting language engineered specifically for cycle-accurate CPU test automation, regression testing, and tool-assisted execution.

> [!IMPORTANT]
> All scripting in 65c02-SIM is powered by a **native C++23 lexer, recursive-descent parser, and AST tree-walking interpreter**. It has zero external dependencies, executes cooperatively alongside the CPU emulation thread, and can be run both within the graphical interface and headlessly in CI/CD pipelines.

```
[Script Source Code (.sim / .script)]
                 │
                 ▼
     [Lang::Lexer (Token Stream)]
                 │
                 ▼
    [Lang::Parser (AST Construction)]
                 │
                 ▼
  [Lang::Interpreter (Tree-Walk Engine)]
                 │
  ┌──────────────┴──────────────┐
  │                             │
  ▼                             ▼
[Environment (Scope & Vars)]   [EmulatorScriptBindings ("emu.*")]
                                ├── Core Emulation (Step, Pause, Reset)
                                ├── Bus Memory Access (Read, Write, Direct)
                                ├── CPU Register Mutators (A, X, Y, PC, SP)
                                └── Hardware Peripherals (GPU, SD, Breakpoints)
```

---

## 2. Language Grammar & Architecture

The scripting language features a procedural, C-like syntax supporting lexical scoping, statement blocks (`{ ... }`), control flow (`if`/`else`, `while`, `break`, `continue`), direct CPU register and memory bindings, and native invocation of registered C++ built-in functions (`emu.*`).

> [!NOTE]
> The grammar does not implement user-defined function declarations (`fn`/`def`/`function`); scripts execute procedurally from top to bottom, manipulating variables and dispatching commands via the built-in `emu.*` library.

### 2.1 Lexical Elements & Tokens

[`System::Hardware::Scripting::Lang::TokenType`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Scripting/Lang/Token.h) defines the language grammar:

- **Keywords**: `let`, `if`, `else`, `while`, `break`, `continue`, `true`, `false`, `print`, `changed`.
- **Numeric Literals**:
  - **Hexadecimal (Dollar prefix)**: `$8000`, `$FF`, `$0200`
  - **Hexadecimal (C prefix)**: `0x8000`, `0xFF`, `0x0200`
  - **Decimal**: `0`, `42`, `1000000`, `-5`
- **String Literals**: `"Double quoted UTF-8 text"`
- **Boolean Literals**: `true`, `false`
- **Operators**:
  - Arithmetic: `+`, `-`, `*`, `/`, unary `-`
  - Comparison: `==`, `!=`, `<`, `<=`, `>`, `>=`
  - Logical: `&&`, `||`, `!`
  - Assignment: `=`
  - Member access: `.`
  - Array / Memory indexing: `[` `]`
- **Comments**: Single-line comments begin with `//` and extend to end of line.

### 2.2 AST Node Hierarchy

The parser builds an Abstract Syntax Tree composed of [`Stmt`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Scripting/Lang/AST.h#L45) and [`Expr`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Scripting/Lang/AST.h#L35) nodes:

#### Statements
- [`VarStmt`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Scripting/Lang/AST.h#L146): Variable declaration (`let name = expr;`).
- [`BlockStmt`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Scripting/Lang/AST.h#L154): Scoped block statement (`{ stmt1; stmt2; }`).
- [`IfStmt`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Scripting/Lang/AST.h#L161): Conditional branch (`if (cond) { ... } else { ... }`).
- [`WhileStmt`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Scripting/Lang/AST.h#L170): Loop statement (`while (cond) { ... }`).
- [`BreakStmt`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Scripting/Lang/AST.h#L178) & [`ContinueStmt`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Scripting/Lang/AST.h#L182): Loop control flow.
- [`PrintStmt`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Scripting/Lang/AST.h#L139): Formatted text output (`print expr;`).
- [`ExpressionStmt`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Scripting/Lang/AST.h#L132): Expression evaluated for side-effects (`emu.step();`).

#### Expressions
- [`BinaryExpr`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Scripting/Lang/AST.h#L57): Binary operations (`a + b`, `x == $20`, `a && b`).
- [`UnaryExpr`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Scripting/Lang/AST.h#L66): Logical NOT (`!flag`) and negation (`-val`).
- [`LiteralExpr`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Scripting/Lang/AST.h#L74): Evaluates immediate constants.
- [`VariableExpr`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Scripting/Lang/AST.h#L82): Variable identifier lookups.
- [`AssignExpr`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Scripting/Lang/AST.h#L88): Reassignment (`x = x + 1;`).
- [`CallExpr`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Scripting/Lang/AST.h#L96): Function calls (`emu.wait_cycles(100);`).
- [`IndexExpr`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Scripting/Lang/AST.h#L113): Direct memory indexing (`mem[$0200]`).
- [`ChangedExpr`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Scripting/Lang/AST.h#L122): Delta-change tracking operator (`expr changed`).

---

## 3. Language Features & Syntax Examples

### 3.1 Variables and Scoping
Variables are defined using the `let` keyword. They are dynamically typed and lexically scoped:

```javascript
let address = $8000;
let counter = 0;
let name = "Sprite Test";

{
    let localVal = address + counter;
    print localVal;
}
// localVal is now out of scope
```

### 3.2 Control Flow
Standard `if`/`else` and `while` constructs govern script logic:

```javascript
let i = 0;
while (i < 10) {
    if (i == 5) {
        print "Halfway mark reached!";
    } else {
        print "Iterating:";
        print i;
    }
    i = i + 1;
}
```

### 3.3 Direct Hardware Variables
The runtime environment directly maps CPU registers and bus memory into the global scope:

| Variable | Description |
| :--- | :--- |
| `PC`, `pc` | Program Counter (16-bit address) |
| `A`, `a` | Accumulator Register (8-bit) |
| `X`, `x` | X Index Register (8-bit) |
| `Y`, `y` | Y Index Register (8-bit) |
| `SP`, `sp` | Stack Pointer (8-bit, `$00-$FF`) |
| `C`, `Z`, `I`, `D`, `B`, `V`, `N` | CPU Status Flags (0 or 1) |
| `mem[addr]` / `MEM[addr]` | Direct read access to memory bus |

```javascript
// Check CPU registers directly
if (A == $FF && Z == 0) {
    print "Accumulator saturated!";
}

// Read from memory directly
if (mem[$0200] == 42) {
    print "Zero page variable verified.";
}
```

---

## 4. Emulator API Reference (`emu.*`)

All interaction with the simulated system is exposed via the built-in [`EmulatorScriptBindings`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Scripting/EmulatorScriptBindings.h) library:

### 4.1 Execution Control

| Function Signature | Description |
| :--- | :--- |
| `emu.pause()` | Halts background CPU execution immediately. |
| `emu.resume()` | Resumes continuous background CPU execution. |
| `emu.reset()` | Triggers a hardware reset vector (`$FFFC-$FFFD`) and updates CPU state. |
| `emu.step()` | Executes exactly one complete 65C02 instruction (all clock cycles). |
| `emu.wait_cycles(count)` | Advances emulation until `count` clock cycles have executed. |
| `emu.wait_instructions(count)` | Advances emulation until `count` instructions have completed. |

### 4.2 Memory Bus Operations

| Function Signature | Description |
| :--- | :--- |
| `emu.read_mem(address)` | Reads an 8-bit byte from the memory bus (triggers device read hooks). |
| `emu.write_mem(address, value)` | Writes an 8-bit byte to the memory bus (triggers device write hooks). |
| `emu.write_mem_direct(address, value)` | Safely writes byte to RAM without firing MMIO hooks. |
| `emu.read_mem_block(address, size)` | Reads a sequential block of `size` bytes, returning a string. |
| `emu.write_mem_block(address, string)`| Writes sequential characters from `string` to memory via bus. |
| `emu.write_mem_block_direct(addr, str)`| Directly writes character block into RAM without MMIO hooks. |
| `emu.load_bin(path, address)` | Loads raw binary file from disk into memory starting at `address`. |

### 4.3 CPU Register Access

| Function Signature | Description |
| :--- | :--- |
| `emu.get_pc()`, `emu.set_pc(val)` | Get or set the 16-bit Program Counter. |
| `emu.get_a()`, `emu.set_a(val)` | Get or set the 8-bit Accumulator. |
| `emu.get_x()`, `emu.set_x(val)` | Get or set the 8-bit X Register. |
| `emu.get_y()`, `emu.set_y(val)` | Get or set the 8-bit Y Register. |
| `emu.get_sp()`, `emu.set_sp(val)` | Get or set the 16-bit / 8-bit Stack Pointer. |
| `emu.get_status()`, `emu.set_status(val)` | Get or set the 8-bit Status Register flags (`NV-BDIZC`). |
| `emu.trigger_irq()` | Asserts an active-low Maskable Interrupt (`/IRQ`). |
| `emu.trigger_nmi()` | Asserts a Non-Maskable Interrupt edge (`/NMI`). |

### 4.4 Hardware Configuration

| Function Signature | Description |
| :--- | :--- |
| `emu.set_gpu_enabled(bool)` | Enables or disables hardware graphics rendering. |
| `emu.set_sd_enabled(bool)` | Enables or disables SPI SD Card peripheral. |
| `emu.set_cycle_accurate(bool)` | Toggles between instruction-step and cycle-accurate mode. |
| `emu.clear_breakpoints()` | Removes all active breakpoints from the Breakpoint Manager. |
| `emu.add_breakpoint()` | Registers a new breakpoint trap. |

### 4.5 Canonical API Mapping & Test Automation Equivalents

Developers writing test automation scripts or transitioning from other emulator environments can use the following mapping:

| Canonical Operation | Native Scripting Implementation | Usage Example |
| :--- | :--- | :--- |
| `step()` | `emu.step()` | Advances one full CPU instruction. |
| `reset()` | `emu.reset()` | Resets CPU registers and vectors. |
| `read(addr)` | `emu.read_mem(addr)` or `mem[addr]` | `let val = mem[$0200];` |
| `write(addr, val)` | `emu.write_mem(addr, val)` or `emu.write_mem_direct(addr, val)` | `emu.write_mem($8000, $EA);` |
| `get_reg(name)` | Direct variable or `emu.get_<reg>()` | `if (A == $50)` or `emu.get_pc()` |
| `set_reg(name, val)`| `emu.set_<reg>(val)` | `emu.set_a($00);` `emu.set_pc($8000);` |
| `load_rom(path, addr)` | `emu.load_bin(path, addr)` | `emu.load_bin("rom.bin", $8000);` |
| `print(value)` | Statement: `print <expr>;` | `print "Passed!";` `print A;` |
| `assert(condition)` | Conditional check with halt | `if (!(A == $42)) { print "FAIL"; emu.pause(); }` |
| `save_state()` / `load_state()` | Host CLI / UI level | Automated via `--rom` and `--dump-mem` in CI/CD. |

### 4.6 ScriptEngine & Cooperative Abortion

The scripting backend ([`Hardware::ScriptEngine`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Scripting/ScriptEngine.h)) executes scripts safely without blocking the UI or CPU thread:
- **`LoadAndRun(path)`**: Reads the source file, triggers lexing and parsing, queues statements into `pendingStmts`, and resumes execution.
- **`Stop()`**: Signals `abortRequested = true`. Long-running operations (`emu.step()`, `emu.wait_cycles()`, and `while` loops) check this flag cooperatively on every step and throw `RuntimeError("Script aborted")`.
- **`mirrorToStdout`**: When running headlessly via `--script`, all text passed to `print` statements is forwarded directly to host `std::cout`.
- **Thread Synchronization**: State mutations from the script thread execute cleanly through `emu.WaitUntilSafeToMutate()` to ensure memory integrity.

---

## 5. Running Scripts

### 5.1 Graphical UI Script Console
1. Open the **Settings** menu inside the emulator Control Window.
2. Under **Scripting**, click **Load & Run Script (.sim / .script)**.
3. The **Script Console** window ([`ScriptConsoleWindow.cpp`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Frontend/GUI/ScriptConsoleWindow.cpp)) will open automatically, displaying text printed by the script in real time.
4. Click **Stop Script** at any time to abort long-running scripts safely.

```
┌─────────────────────────────────────────────────────────────┐
│ Script Console                               [Clear] [Close]│
├─────────────────────────────────────────────────────────────┤
│ [Script] Initializing Memory Verification Test...           │
│ [Script] Flashing ROM image to $8000                        │
│ [Script] Stepping 10,000 cycles...                          │
│ [Script] SUCCESS: Memory at $0200 verified (Expected: $42)   │
└─────────────────────────────────────────────────────────────┘
```

### 5.2 Headless Execution (CI/CD Automated Tests)
Run scripts without graphical dependencies directly from bash:

```bash
./SIM_65C02 --headless \
            --rom build/program.bin \
            --script scripts/test_suite.sim \
            --dump-mem /tmp/output.bin
```

---

## 6. Practical Script Examples

### 6.1 Automated Memory Verification Test

```javascript
// 1. Reset CPU and pause background execution
emu.pause();
emu.reset();
emu.clear_breakpoints();

print "Injecting test instructions...";
// Assemble LDA #$42, STA $0200, BRK
emu.write_mem($8000, $A9); // LDA #$42
emu.write_mem($8001, $42);
emu.write_mem($8002, $8D); // STA $0200
emu.write_mem($8003, $00);
emu.write_mem($8004, $02);
emu.write_mem($8005, $00); // BRK

// Point PC to start
emu.set_pc($8000);

print "Executing instruction steps...";
emu.step(); // LDA
emu.step(); // STA

// Verify target memory
let result = emu.read_mem($0200);
if (result == $42) {
    print "TEST PASSED: Memory at $0200 equals $42";
} else {
    print "TEST FAILED: Unexpected memory value at $0200:";
    print result;
}
```

### 6.2 Polling for Asynchronous Hardware Events

```javascript
// Wait for a peripheral or interrupt flag to clear
let timeout = 1000;
let done = false;

while (!done && timeout > 0) {
    emu.step();
    timeout = timeout - 1;
    
    // Check VIA Port B handshake
    if (emu.read_mem($6000) == $01) {
        done = true;
    }
}

if (done) {
    print "Hardware signal acknowledged in time!";
} else {
    print "ERROR: Timed out waiting for VIA handshake.";
}
```
