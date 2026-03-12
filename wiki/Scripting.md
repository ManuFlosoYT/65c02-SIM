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

### Example: Run a script headlessly

```bash
./SIM_65C02 --headless --script ScriptingAPI/test_api.py --run-cycles 500
```

---

## Python API Reference (`emu` module)

All scripting is done via the `emu` built-in module, which is automatically available in every script — no import needed... unless PocketPy requires it (use `import emu` to be safe).

### `emu.pause()`
Pauses the emulator's CPU execution. The main render loop continues, but no instructions are executed.

```python
emu.pause()
```

---

### `emu.resume()`
Resumes the CPU execution from a paused state.

```python
emu.resume()
```

---

### `emu.read_mem(address) → int`
Reads a single byte from the emulator's memory bus at the given 16-bit address.

| Parameter | Type | Description |
|---|---|---|
| `address` | `int` | 16-bit address (0x0000–0xFFFF) |

```python
val = emu.read_mem(0x0200)
print(hex(val))  # e.g. 0x42
```

---

### `emu.write_mem(address, value)`
Writes a single byte to the emulator's memory bus.

| Parameter | Type | Description |
|---|---|---|
| `address` | `int` | 16-bit address (0x0000–0xFFFF) |
| `value` | `int` | Byte value (0–255) |

```python
emu.write_mem(0x0200, 42)
```

---

### `emu.wait_cycles(n)`
Blocks the script thread until the emulator has executed `n` more cycles.

- If the CPU is **paused**, it will step it manually for `n` cycles.
- If the CPU is **running**, it will sleep proportionally (based on ms).

| Parameter | Type | Description |
|---|---|---|
| `n` | `int` | Number of cycles to wait |

```python
emu.wait_cycles(1000)
```

---

## Full Example Script

```python
import emu

emu.pause()

# Inject a small 6502 program: LDA #$42, STA $0200, BRK
program = [0xA9, 0x42, 0x8D, 0x00, 0x02, 0x00]
for i, byte in enumerate(program):
    emu.write_mem(0x0000 + i, byte)

# Verify the writes
for i, expected in enumerate(program):
    actual = emu.read_mem(0x0000 + i)
    assert actual == expected, f"Mismatch at {hex(i)}: {hex(actual)} != {hex(expected)}"
    print(f"  0x{i:04X}: {hex(actual)} OK")

emu.resume()
emu.wait_cycles(100)

print("Script finished.")
emu.pause()
```

---

## Architecture Notes

- Scripts run in a **dedicated background thread** so the GUI and emulator aren't blocked.
- All `print()` output is captured by a custom PocketPy callback and pushed to a **thread-safe queue** (`std::deque<std::string>` + `std::mutex`), which the GUI reads each frame and renders in the Script Console window.
- The `ScriptEngine` class lives in `Hardware/Scripting/ScriptEngine.{h,cpp}` and is owned by `Core::Emulator`.
- The Script Console window is implemented in `Frontend/GUI/ScriptConsoleWindow.{h,cpp}`.
