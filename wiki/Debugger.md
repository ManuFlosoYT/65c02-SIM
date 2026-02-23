# Debugger and Memory Profiler

The **65c02-SIM** includes a powerful debugger and a memory profiler designed to help developers analyze their code's execution and performance.

## How to Access

Click the **Debugger** button in the main control panel. The debugger will open in a separate, movable window.

## Window Features

The debugger is a **non-modal window**, which means:

- You can move and resize it at any time.
- It doesn't block the background UI; you can still interact with the control panel or other windows.
- It can remain open while the emulator is running.

## Sidebar Controls

The sidebar on the left provides quick access to frequently used commands:

- **Run/Pause**: Toggle the emulation state.
- **Step**: Execute a single instruction.
- **Reset**: Re-initialize the CPU, GPU, and RAM, loading the ROM again.
- **Exit**: Close the debugger window.

## Debugger Tabs

### 1. Debugger (WIP)

Placeholder for future debugging features such as register inspection and breakpoints.

### 2. Disassembly (WIP)

Placeholder for real-time disassembly of the running code.

### 3. Profiler

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

## Technical Implementation

- **File Location**: `Frontend/GUI/Debugger/`
- **Profiling Logic**: `Frontend/GUI/Debugger/ProfilerWindow.cpp`
- **Hardware Integration**: `Hardware/Mem.h` (using `profilerCounts` array and `profilingEnabled` flag).
