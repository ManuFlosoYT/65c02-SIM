# Breakpoints

The **65c02-SIM** debugger includes a full-featured conditional breakpoint system. Breakpoints allow you to automatically pause emulation when specific conditions are met, such as reaching a particular address, a register holding a certain value, or a memory location being written to.

## How to Access

Open the **Debugger** window and select the **Breakpoints** tab from the sidebar.

## Breakpoint Types

### 1. PC Address

Pauses execution when the Program Counter reaches a specific address.

| Field   | Description                           |
|---------|---------------------------------------|
| Address | 16-bit hex address (e.g., `0x8042`)   |

**Use case**: Pause at the start of a subroutine or at a known problematic instruction.

### 2. Register Condition

Pauses when a CPU register satisfies a comparison.

| Field    | Options                                      |
|----------|----------------------------------------------|
| Register | `A`, `X`, `Y`, `SP`, `PC`                   |
| Operator | `==`, `!=`, `<`, `<=`, `>`, `>=`             |
| Value    | Hex value (8-bit for A/X/Y, 16-bit for SP/PC)|

**Examples**:
- `A == 0xFF` — pause when the accumulator reaches 255
- `SP < 0x0180` — pause on potential stack overflow
- `X >= 0x10` — pause when X register is 16 or higher

### 3. Flag Condition

Pauses when a CPU status flag has a specific value.

| Field | Options                          |
|-------|----------------------------------|
| Flag  | `C`, `Z`, `I`, `D`, `B`, `V`, `N` |
| Value | `0` or `1`                       |

**Examples**:
- `Z == 1` — pause when Zero flag is set
- `C == 0` — pause when Carry flag is clear

### 4. Memory Value Condition

Pauses when the contents of a memory address satisfy a comparison.

| Field    | Description                            |
|----------|----------------------------------------|
| Address  | 16-bit hex address (e.g., `0x2000`)    |
| Operator | `==`, `!=`, `<`, `<=`, `>`, `>=`       |
| Value    | 8-bit hex value (e.g., `0x42`)         |

**Examples**:
- `[0x0042] == 0xFF` — pause when zero-page location `$42` contains `0xFF`
- `[0x2000] != 0x00` — pause when VRAM base is non-zero

### 5. Memory Watchpoint

Pauses whenever a specific memory address is **written to**, regardless of the value written.

| Field   | Description                          |
|---------|--------------------------------------|
| Address | 16-bit hex address (e.g., `0x2000`)  |

**Use case**: Detect unexpected writes to critical memory locations like the stack, zero-page variables, or hardware registers.

> [!NOTE]
> Watchpoints use the Bus write hook system and activate the `Debug=true` path in the emulation loop when present.

## Compound Breakpoints

A single breakpoint can contain **multiple conditions** connected by either **AND** or **OR** logic:

| Logic | Behavior                                      |
|-------|-----------------------------------------------|
| AND   | All conditions must be true simultaneously    |
| OR    | At least one condition must be true           |

**Example (AND)**:
```
A == 0x00 AND Z == 1
```
Pauses only when the accumulator is zero **and** the Zero flag is set.

**Example (OR)**:
```
PC == 0x8042 OR [0x0042] == 0xFF
```
Pauses when execution reaches `$8042` **or** when memory at `$42` equals `$FF`.

### Building Compound Breakpoints

1. Configure the first condition using the form controls.
2. Click **Add Condition** to append it to the pending list.
3. Select **AND** or **OR** from the Logic dropdown.
4. Configure and add additional conditions.
5. Click **Create Breakpoint** to finalize.

## Quick Breakpoint Helpers

The **Quick Breakpoints** section provides one-click buttons that pre-fill the form for common patterns:

| Button          | Effect                                        |
|-----------------|-----------------------------------------------|
| `PC == addr`     | Sets type to PC Address                       |
| `A == val`       | Sets type to Register, register to A, op to ==|
| `X == val`       | Sets type to Register, register to X, op to ==|
| `Y == val`       | Sets type to Register, register to Y, op to ==|
| `[addr] == val`  | Sets type to Memory Value, op to ==           |
| `Write @ addr`   | Sets type to Memory Watchpoint                |
| `Z flag set`     | Sets type to Flag, flag to Z, value to 1      |
| `C flag set`     | Sets type to Flag, flag to C, value to 1      |

After clicking a helper button, fill in the remaining value/address fields and click **Create Breakpoint**.

## Breakpoint Options

| Option    | Description                                                    |
|-----------|----------------------------------------------------------------|
| **Enabled** | Toggle checkbox next to each breakpoint in the list          |
| **Hit Once** | Auto-disables the breakpoint after the first hit            |
| **Label**   | Optional human-readable name for the breakpoint              |

## Breakpoint List

The list displays all defined breakpoints in a table:

| Column     | Description                                    |
|------------|------------------------------------------------|
| ☑          | Enable/disable toggle                          |
| ID         | Auto-assigned unique identifier                |
| Label      | User-defined name or `-`                       |
| Conditions | Human-readable summary of all conditions       |
| Hits       | Number of times the breakpoint has triggered   |
| X          | Delete the breakpoint                          |

Use **Clear All** to remove all breakpoints at once.

## Behavior

- Breakpoints are evaluated **after each instruction** during emulation.
- When a breakpoint hits, emulation **pauses immediately** at the exact instruction boundary.
- Breakpoints are **volatile** — they are not saved in savestates and are lost when the emulator is closed.
- Having active breakpoints forces the `Debug=true` code path, which has a minimal performance overhead.

## Scripting API Integration

Breakpoints can also be managed programmatically using the **Python Scripting API**. This allows for automated regressions, TAS development, and advanced debugging.

Common tasks available via script:
- Add a breakpoint with a complex `dict` configuration.
- List all active breakpoints and their hit counts.
- Remove or clear breakpoints during execution.

For the full list of scriptable breakpoint functions and constants, see the [Scripting documentation](file:///home/manu/Documents/GitHub/65c02-SIM/wiki/Scripting.md#8-advanced-breakpoints).

---

## Technical Details

- **Backend**: `Hardware/Core/BreakpointManager.h` / `BreakpointManager.cpp`
- **UI**: `Frontend/GUI/Debugger/BreakpointWindow.h` / `BreakpointWindow.cpp`
- **Scripting**: `Hardware/Scripting/ScriptEngine.cpp` (`py_emu_add_breakpoint`, etc.)
- **Integration**: Breakpoints are evaluated in `Emulator::EmulateSlice()` after each instruction step.
- **Watchpoints**: Use the `Bus::AddGlobalWriteHook()` mechanism to receive write notifications.
