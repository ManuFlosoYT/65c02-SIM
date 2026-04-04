# Scripting System & Headless Mode

The emulator includes a built-in **Python scripting engine** powered by [PocketPy](https://github.com/pocketpy/pocketpy) — a lightweight, embeddable Python 3 interpreter. Scripts can control the emulator programmatically, making them useful for:

- **Tool-Assisted Speedruns (TAS)** — inject inputs and steer execution at the cycle level.
- **Test-Driven Development** — write automated tests that validate memory state after running a program.
- **Overlays / Debugging** — read live values from RAM and react to them in real time.

---

## Running a Script from the GUI

1. Open the emulator and go to the **Settings** panel (gear icon or dropdown).
2. Scroll to the **Scripting** section.
3. Click **Load & Run Script (.py)** and select your `.py` file.

When a script is loaded, the emulator is automatically **paused** and then the script thread starts. A **Script Console** window will appear, streaming every `print()` call from your script in real time — just like a terminal.

You can stop the script at any time with the **Stop Script** button that appears while a script is running.

> **Tip:** Always call `emu.pause()` at the end of your script if you don't want the CPU to keep running after the script finishes.

---

## Headless Mode (CLI / CI)

Headless mode lets you run the emulator **without any graphical interface**, directly from the command line. This is ideal for CI/CD pipelines, automated tests, and batch processing.

### Usage

```bash
./SIM_65C02 [OPTIONS]
```

### Options

| Option | Description |
|---|---|
| `--headless` | Skip GUI initialization and run in CLI mode. |
| `--rom <path>` | Load a binary ROM file (also works as positional arg). |
| `--script <path>` | Load and execute a Python script. |
| `--run-cycles <N>` | Advance the CPU exactly N cycles after startup. |
| `--dump-mem <file>` | After execution, dump the full 64KB RAM to a binary file. |

### Example: TDD with CI

```bash
# Run a program for 10000 cycles, and dump memory
./SIM_65C02 --headless --rom build/program.bin --run-cycles 10000 --dump-mem result.bin

# Validate expected value at address 0x0200 using Python
python3 -c "
data = open('result.bin','rb').read()
assert data[0x0200] == 42, f'Expected 42, got {data[0x0200]}'
print('Test passed!')
"
```

---

## Python API Reference (`emu` module)

All scripting is done via the `emu` built-in module, which is automatically available in every script.

### 1. Basic Control
- **`emu.pause()`**: Pauses CPU execution.
- **`emu.resume()`**: Resumes CPU execution.
- **`emu.reset()`**: Performs a hardware reset (vectors at `$FFFC-$FFFD`).

### 2. Memory Bus
- **`emu.read_mem(address: int) -> int`**: Reads a single byte (0-255).
- **`emu.write_mem(address: int, value: int)`**: Writes a single byte.

### 3. CPU Registers
Full programmatic access to all CPU registers:
- `emu.get_a()` / `emu.set_a(val)`
- `emu.get_x()` / `emu.set_x(val)`
- `emu.get_y()` / `emu.set_y(val)`
- `emu.get_pc()` / `emu.set_pc(val)`
- `emu.get_sp()` / `emu.set_sp(val)`
- `emu.get_status()` / `emu.set_status(val)`

### 4. Execution Timing
- **`emu.wait_cycles(n: int)`**: Blocks script thread until CPU has executed `n` cycles.
- **`emu.wait_instructions(n: int)`**: Blocks until `n` instructions are executed.
- **`emu.step_instruction()`**: Advances the CPU by exactly one instruction.
- **`emu.step()`**: Advances the CPU by one phase (internal cycle).

### 5. Hardware Configuration
Toggle peripherals programmatically to optimize performance or test failure cases:
- `emu.set_gpu_enabled(bool)`
- `emu.set_sd_enabled(bool)`
- `emu.set_esp_enabled(bool)`
- `emu.set_cycle_accurate(bool)`: Toggles cycle-accurate mode (high latency vs. high performance).

### 6. Metrics & Performance
- **`emu.get_cycles() -> int`**: Returns the count of cycles since power-on.
- **`emu.get_ips() -> int`**: Returns the current instructions per second.
- **`emu.set_target_ips(ips: int)`**: Dynamically changes the emulation speed limit.

### 7. Multimedia & Input
- **`emu.inject_key(key: str)`**: Injects a key character into the emulator's keyboard buffer.
- **`emu.start_audio_recording(filename: str)`**: Starts recording SID audio to a WAV file.
- **`emu.stop_audio_recording()`**: Stops and finalizes the audio recording.
- **`emu.load_cartridge(path: str)`**: Loads and mounts a `.cart` or `.bin` file into the emulator at runtime.

### 8. Advanced Breakpoints
The scripting API allows creating complex, multi-condition breakpoints that are identical to those available in the UI.

- **`emu.add_breakpoint(config: dict) -> int`**: Creates a new breakpoint and returns its unique ID.
- **`emu.remove_breakpoint(id: int)`**: Deletes a breakpoint.
- **`emu.list_breakpoints() -> list`**: Returns a list of dictionaries with all currently active breakpoints.
- **`emu.clear_breakpoints()`**: Removes all breakpoints.

#### Breakpoint Config Dictionary Structure
```python
id = emu.add_breakpoint({
    "label": "My Complex Breakpoint",
    "enabled": True,
    "compoundOp": emu.LOGIC_AND,
    "conditions": [
        {
            "type": emu.BP_TYPE_REG,
            "reg": emu.REG_A,
            "op": emu.OP_EQUAL,
            "value": 0xFF
        },
        {
            "type": emu.BP_TYPE_PC,
            "address": 0x8050
        }
    ]
})
```

#### Breakpoint Constants
| Constant Category | Values |
|---|---|
| **`BP_TYPE`** | `PC`, `REG`, `FLAG`, `MEM`, `WATCH` |
| **`REG`** | `A`, `X`, `Y`, `SP`, `PC` |
| **`OP`** | `EQUAL`, `NOT_EQUAL`, `LESS`, `LESS_EQUAL`, `GREATER`, `GREATER_EQUAL` |
| **`LOGIC`** | `AND`, `OR` |

---

## Full Example: Automated Testing
This script sets up a breakpoint, injects code, and verifies that the CPU stops at the right time.

```python
import emu

# 1. Setup Environment
emu.pause()
emu.clear_breakpoints()
emu.reset()

# 2. Inject Code: LDA #$99, STA $0200, BRK
program = [0xA9, 0x99, 0x8D, 0x00, 0x02, 0x00]
for i, byte in enumerate(program):
    emu.write_mem(0x0000 + i, byte)

# 3. Add conditional breakpoint for STA instruction
emu.add_breakpoint({
    "label": "Stop on STA",
    "conditions": [
        {
            "type": emu.BP_TYPE_PC,
            "address": 0x0002
        }
    ]
})

# 4. Execute and Wait
emu.set_pc(0x0000)
emu.resume()
emu.wait_cycles(100) # Give it time to hit

# 5. Verify
if emu.get_a() == 0x99:
    print("Success: Final Accumulator value is $99")
else:
    print(f"Error: Final Accumulator value is {hex(emu.get_a())}")

emu.pause()
```

---

## Architecture Notes

- Scripts run in a **dedicated background thread** so the GUI and emulator aren't blocked.
- All `print()` output is captured by a custom PocketPy callback and pushed to a thread-safe queue.
- The `ScriptEngine` class lives in `Hardware/Scripting/ScriptEngine.{h,cpp}`.
