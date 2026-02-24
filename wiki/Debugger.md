# Debugger and Memory Profiler

The **65c02-SIM** includes a powerful debugger and a memory profiler designed to help developers analyze their code's execution and performance.

## How to Access

Click the **Debugger** button in the main control panel. The debugger will open in a separate, movable window.

## Window Features

The debugger is a **non-modal window**, which means:

- You can move it at any time.
- It is **not resizable** to maintain a consistent layout.
- It doesn't block the background UI; you can still interact with the control panel or other windows.
- It can remain open while the emulator is running.

## Sidebar Controls

The sidebar on the left provides quick access to frequently used commands:

- **Run/Pause**: Toggle the emulation state.
- **Step**: Execute a single instruction.
- **Reset**: Re-initialize the CPU, GPU, and RAM, loading the ROM again.
- **Exit**: Close the debugger window.

## Debugger Tabs

### 1. Disassembly

The **Real-Time Disassembler** translates binary data from memory back into human-readable 65C02 assembly language.

- **Address & Bytes**: Shows the memory address and the raw bytes (hex) of each instruction.
- **Mnemonic & Operands**: Displays the 65C02 instruction name and its corresponding parameters (using various addressing modes like `$1234`, `#$55`, `($20,X)`, etc.).
- **Live Highlight**: The instruction currently at the **Program Counter (PC)** is highlighted in blue, allowing you to follow the execution flow in real-time.
- **Dynamic Fetching**: Instructions are disassembled on-the-fly around the current PC as you step through the code or while the emulator is running.

### 2. Profiler

The **Memory Profiler** provides a visual representation of memory access patterns across the entire 64 KB address space.

#### Heatmap Visualization

- **Layout**: A 256×256 grid where each pixel represents a memory address.
  - X axis: `addr & 0xFF`
  - Y axis: `addr >> 8`
- **Color Scale**: Uses a **black to red** gradient.
  - **Black**: No access.
  - **Dark Red → Bright Red**: Increasing access frequency.
- **Interactive Tooltips**: Hover over any pixel on the heatmap to see the hex address and the exact number of accesses recorded.
- **Normalization with Logarithmic Scaling**: To handle hotspots (like the zero page or stack) that would otherwise make everything else appear black, the profiler uses a logarithmic scale:
  `intensity = log(count + 1) / log(max_count + 1)`
  This allows low-frequency accesses in the ROM or general RAM to remain visible.

#### Performance & Management

- **Hardware Hooking**: Profiling is performed directly in the `Mem::Read` and `Mem::Write` hooks.
- **Intelligent Tracking**: For performance, profiling data is _only_ collected when the **Profiler** tab is active and the window is open.
- **Clear Data**: Use the **Clear Data** button at the top-left of the heatmap to reset all access counters.
- **Export as BMP**: Save a high-resolution 256×256 BMP image of the current heatmap for external analysis. The exported image preserves the logarithmic scaling used in the display.
- **Export RAW**: Save a full dump of the 64 KB memory access counters to a `.txt` file. Each line follows the format `0xXXXX: <count>`.

### 3. Debugger

The **Hardware Debugger** allows for direct inspection and modification of the system state during execution or while paused.

#### CPU Register Editing

All core CPU registers can be modified in real-time:

- **PC (Program Counter)**: Change the starting address for code execution.
- **SP (Stack Pointer)**: Manually adjust the stack location.
- **A, X, Y**: Modify the values in the accumulator and index registers.

> [!TIP]
> Changes to registers take effect immediately on the next clock cycle or instruction step.

#### Memory Editor

A comprehensive interface to view and modify the entire 64 KB memory space (RAM and ROM).

- **Interactive Table**: Displays memory in a 16-column grid for easy cross-referencing.
- **Direct Entry**: Click on any hex value to type a new byte value (00-FF).
- **Infinite Scrolling**: Uses an optimized list clipper to allow smooth navigation through the full address range without performance loss.

#### Memory Tools

Located at the bottom of the debugger window, these tools allow for batch operations:

- **Range Selection**: Define a Start and End address in hexadecimal.
- **Fill Random Junk**: Populate the specified range with random bytes. This is particularly useful for stress-testing memory management or visualizing memory patterns in the Profiler.

## Technical Implementation

- **File Location**: `Frontend/GUI/Debugger/`
- **Profiling Logic**: `Frontend/GUI/Debugger/ProfilerWindow.cpp`
- **Disassembly Logic**: `Hardware/CPU/Disassembler.cpp`
- **Disassembly UI**: `Frontend/GUI/Debugger/DisassemblerWindow.cpp`
- **Hardware Integration**: `Hardware/Mem.h` (using `profilerCounts` array and `profilingEnabled` flag).
