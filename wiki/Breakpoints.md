# Breakpoints & Execution Traps

**Source Files:**  
- Engine Backend: [`src/Hardware/Core/BreakpointManager.h`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Hardware/Core/BreakpointManager.h) & [`BreakpointManager.cpp`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Hardware/Core/BreakpointManager.cpp)  
- Condition Evaluator: [`src/Hardware/Core/BreakpointConditionEvaluator.h`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Hardware/Core/BreakpointConditionEvaluator.h) & [`BreakpointConditionEvaluator.cpp`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Hardware/Core/BreakpointConditionEvaluator.cpp)  
- User Interface: [`src/Frontend/GUI/Debugger/BreakpointWindow.cpp`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Frontend/GUI/Debugger/BreakpointWindow.cpp)

---

## 1. Overview

The **65c02-SIM** breakpoint system provides advanced execution trapping, enabling developers to halt the CPU based on instruction addresses, register states, status flags, memory modifications, or complex algebraic expressions.

Breakpoints are evaluated cleanly at instruction boundaries inside the core execution loop. When a breakpoint triggers, emulation pauses immediately, the exact Program Counter is preserved, and the UI shifts focus to the disassembler and register viewer.

```
                  [65C02 Instruction Step Completed]
                                  │
                                  ▼
                     [BreakpointManager::Evaluate]
                                  │
         ┌────────────────────────┴────────────────────────┐
         │                                                 │
[Fast Path Check: O(1)]                          [Complex Evaluator]
  std::bitset<65536>                               ├── Register Conditions
  Matches PC Address?                              ├── Status Flag Checks
         │                                         ├── Memory Watchpoints
         ├─► YES ──┐                               └── AST Expression Engine
         │         │                                       │
         └─► NO    │                                       ├─► HIT ──┐
                   │                                       │         │
                   │                                       └─► MISS  │
                   │                                                 │
                   ▼                                                 ▼
             [Continue Run]                                   [Pause Emulation]
                                                              Increment Hit Counter
                                                              Auto-disable if HitOnce
```

---

## 2. High-Performance Evaluation Architecture

To avoid degrading emulation performance from 10+ MHz down to kHz speeds when thousands of instructions execute per frame, the [`BreakpointManager`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Hardware/Core/BreakpointManager.h) employs a multi-tiered evaluation pipeline:

### 2.1 Fast-Path Bitset (`fastPathBreakpoints`)
For standard Program Counter address breakpoints (e.g. `PC == $8050`), the manager maintains a 65,536-bit direct lookup table ([`std::bitset<65536>`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Hardware/Core/BreakpointManager.h#L60)).
- Evaluation requires only an $O(1)$ bit test: `fastPathBreakpoints.test(cpu.PC)`.
- Eliminates mutex locks, heap allocations, and condition iteration for address traps.

### 2.2 Atomic Fast Bypasses
The engine maintains relaxed atomic flags:
- `hasAnyBreakpoints`: If `false`, the emulation slice completely skips breakpoint logic.
- `hasComplexBreakpoints`: If `false`, non-PC condition evaluation loops are bypassed.
- `hasWatchpoints`: Bypasses bus memory write hook dispatching when no watchpoints exist.

---

## 3. Breakpoint Types & Access Modes

The system defines six core condition primitives in [`Hardware::BreakpointType`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Hardware/Core/BreakpointConditionEvaluator.h#L18):

| Breakpoint Type | Enum Identifier | Access Type | Description | Trigger Example |
| :--- | :--- | :---: | :--- | :--- |
| **PC Address** | `PCAddress` | **Execution** | Halts when the Program Counter matches a specific 16-bit address ($O(1)$ fast-path). | `PC == $8040` |
| **Register Condition** | `RegisterCondition` | **Internal** | Compares CPU registers (`A`, `X`, `Y`, `SP`, `PC`) using relational operators. | `A >= $80` or `SP < $0180` |
| **Flag Condition** | `FlagCondition` | **Internal** | Tests status flags (`C`, `Z`, `I`, `D`, `B`, `V`, `N`) against `0` or `1`. | `Z == 1` or `C == 0` |
| **Memory Condition** | `MemoryCondition` | **Read / State** | Compares the 8-bit contents of a memory address with a value at instruction retirement. | `[$0200] == $FF` |
| **Memory Watchpoint**| `MemoryWatchpoint` | **Write** | Intercepts bus write cycles via `bus.AddGlobalWriteHook` (`NotifyWrite`). | `Write @ $6000` |
| **Custom Expression**| `Expression` | **Composite** | Evaluates a complex boolean expression through the AST interpreter. | `A == $50 && (X > 2 \|\| Y == 0)` |

### 3.1 Access Mode Mechanics: Execution vs. Write vs. State
1. **Execution Traps (`PCAddress`)**: Tested before each instruction executes using `fastPathBreakpoints.test(cpu.PC)`. Triggers in $\mathcal{O}(1)$ time without taking locks.
2. **Write Watchpoints (`MemoryWatchpoint`)**: Hooks into the memory bus via `bus.AddGlobalWriteHook()`. Any write cycle during instruction processing fires `BreakpointManager::NotifyWrite(address, value)`, setting `watchpointTriggered = true` and `watchpointAddress = address`. Upon evaluating the breakpoint, `ClearWatchpointHit()` clears the flag for the next cycle.
3. **Memory State Inspection (`MemoryCondition`)**: Reads memory using non-intrusive direct access (`bus.ReadDirect(address)`), ensuring hardware I/O registers with read-side-effects (such as ACIA or VIA status flags) are not accidentally acknowledged or modified during inspection.

### Supported Relational Operators

All register and memory condition comparisons support standard relational operators:

| Operator | Enum Value | Description |
| :---: | :--- | :--- |
| `==` | `CompareOp::Equal` | Left-hand side equals right-hand side. |
| `!=` | `CompareOp::NotEqual` | Left-hand side does not equal right-hand side. |
| `<` | `CompareOp::Less` | Left-hand side is strictly less than right-hand side. |
| `<=` | `CompareOp::LessEqual` | Left-hand side is less than or equal to right-hand side. |
| `>` | `CompareOp::Greater` | Left-hand side is strictly greater than right-hand side. |
| `>=` | `CompareOp::GreaterEqual` | Left-hand side is greater than or equal to right-hand side. |

---

## 4. Complex Expression Grammar & Syntax

The breakpoint engine includes an embedded expression evaluator backed by the simulator's custom scripting language lexer and parser ([`System::Hardware::Scripting::Lang`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Scripting/Lang/AST.h)). Expressions are compiled into an Abstract Syntax Tree (AST) when the breakpoint is created and evaluated against the live CPU and Bus contexts without runtime string manipulation.

### 4.1 Syntax Rules & Symbols

```ebnf
Expression     ::= LogicalOr
LogicalOr      ::= LogicalAnd ( "||" LogicalAnd )*
LogicalAnd     ::= Equality ( "&&" Equality )*
Equality       ::= Comparison ( ( "==" | "!=" ) Comparison )*
Comparison     ::= Term ( ( "<" | "<=" | ">" | ">=" ) Term )*
Term           ::= Factor ( ( "+" | "-" ) Factor )*
Factor         ::= Unary ( ( "*" | "/" ) Unary )*
Unary          ::= ( "!" | "-" ) Unary | Primary
Primary        ::= Number | Identifier | MemoryAccess | "(" Expression ")" | ChangedExpr
MemoryAccess   ::= ( "mem" | "MEM" ) "[" Expression "]"
ChangedExpr    ::= Expression "changed"
```

> [!IMPORTANT]
> Within custom expressions (`BreakpointType::Expression`), memory indexing MUST be prefixed with `mem` or `MEM` (e.g. `mem[$0200]`). The bracketed shorthand `[0x0200] == 0xFF` is the summary string generated by the GUI for structured `MemoryCondition` items, but a bare `[` cannot start an expression in the parser.

### 4.2 Available Identifiers

1. **CPU Registers (Case-Insensitive)**:
   - `A` / `a`: Accumulator (8-bit, $0-255$).
   - `X` / `x`: X Index Register (8-bit, $0-255$).
   - `Y` / `y`: Y Index Register (8-bit, $0-255$).
   - `SP` / `sp`: Stack Pointer ($0-255$, points into page `$0100-$01FF`).
   - `PC` / `pc`: Program Counter (16-bit, $0-65535$).

2. **CPU Status Flags (0 or 1)**:
   - `C` / `c`: Carry flag
   - `Z` / `z`: Zero flag
   - `I` / `i`: Interrupt Disable flag
   - `D` / `d`: Decimal mode flag
   - `B` / `b`: Break command flag
   - `V` / `v`: Overflow flag
   - `N` / `n`: Negative flag

3. **Memory Access & Pointer Dereferencing**:
   - `mem[address]` or `MEM[address]`: Reads the 8-bit value at `address` from the memory bus.
   - Pointer Dereference: `mem[mem[$0020] + (mem[$0021] * 256)]` dynamically resolves 16-bit zero-page indirect addresses.

4. **Numeric Literals**:
   - Hexadecimal: `$1000` or `0x1000`
   - Decimal: `42`, `255`, `65535`
   - Boolean: `true`, `false`

5. **Transition Tracking (`changed`)**:
   - Appending `changed` to an expression evaluates to `true` only when the calculated value differs from the previous instruction step.
   - Example: `mem[$0200] changed` triggers when a variable in RAM is altered.

### 4.3 Expression Examples

#### Subroutine Parameter Validation
```c
PC == $8050 && A != $00 && X > 4
```
Halts execution when the subroutine at `$8050` is called with invalid parameters ($A \neq 0$ and $X > 4$).

#### Stack Overflow Trap
```c
SP < $0140
```
Halts execution if the hardware stack grows downward beyond address `$0140`, catching deep recursive overflows before Zero Page corruption occurs.

#### Memory Value Check
```c
mem[$0200] == $FF && (Z == 1 || C == 0)
```
Halts when memory location `$0200` equals `$FF` while either the Zero flag is asserted or the Carry flag is clear.

#### 16-Bit Pointer Dereferencing
```c
mem[mem[$0020] + (mem[$0021] * 256)] == $FF
```
Dynamically reconstructs a 16-bit little-endian vector stored at zero-page locations `$0020` (low byte) and `$0021` (high byte), halting when the pointed-to memory byte equals `$FF`.

---

## 5. Compound Breakpoints & Logic Operations

A single breakpoint entry can combine multiple conditions using either **AND** or **OR** compound logic ([`Hardware::LogicOp`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Hardware/Core/BreakpointConditionEvaluator.h#L30)):

- **`LogicOp::And`**: All listed conditions must evaluate to `true` simultaneously for the trap to trigger.
- **`LogicOp::Or`**: The trap triggers if any single condition evaluates to `true`.

```
┌─────────────────────────────────────────────────────────────────────────────┐
│ Breakpoint #2 [Label: "Detect Buffer Overrun"]                              │
│ Logic: [AND]                                                                │
│ ├── Condition 1: Register X >= $80                                          │
│ └── Condition 2: Memory [$0200 + X] != $00                                  │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## 6. Breakpoint Controls & Options

Each breakpoint entry supports individual operational flags:

| Option | Struct Member | Default | Functionality |
| :--- | :--- | :---: | :--- |
| **Enabled** | `breakpoint.enabled` | `true` | Allows temporarily bypassing a breakpoint without deleting its rules. |
| **Hit Once** | `breakpoint.hitOnce` | `false`| One-shot breakpoint. Automatically disables itself after triggering once. |
| **Hit Counter** | `breakpoint.hitCount` | `0` | Monotonically increments every time the breakpoint condition is satisfied. |
| **Label** | `breakpoint.label` | `""` | Optional human-readable name displayed in the debugger table. |

---

## 7. IDE Line Breakpoints Integration

When source code is edited or debug symbols are loaded via the integrated IDE or compiler toolchain:
1. Clicking the gutter margin next to any line number in [`IDEWindow.cpp`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Frontend/GUI/IDEWindow.cpp) toggles a line breakpoint.
2. The IDE queries [`Control::SymbolTable::GetLineForAddress()`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Hardware/Core/SymbolTable.h#L98) to translate the source file and line into an exact 16-bit address.
3. An active `PCAddress` breakpoint is registered in `BreakpointManager`.
4. When hit, the IDE editor automatically highlights the source code line with an execution arrow.

---

## 8. Breakpoint Management via Scripting API

Breakpoints can also be queried, created, and deleted programmatically through the custom scripting engine:

```javascript
// Clear existing traps
emu.clear_breakpoints();

// Add a breakpoint programmatically
emu.add_breakpoint();

// Verify hit count or step until trigger
while (emu.get_pc() != $8050) {
    emu.step();
}
print "Reached target address:";
print emu.get_pc();
```
