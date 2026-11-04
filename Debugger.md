# Integrated Debugger & Analysis Suite

**Source Directory:** [`src/Frontend/GUI/Debugger/`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Frontend/GUI/Debugger/)  
**Technologies:** Dear ImGui · OpenGL 3.3 · Modern C++23

---

## 1. Overview

The **65c02-SIM** debugger suite provides deep, non-intrusive visibility into the simulated machine's internal hardware and bus operations. Rather than treating the 65C02 CPU as a black box, the debugger offers real-time hardware telemetry across CPU registers, memory access heatmaps, bus logic timing, digital signal wave analyzers, and peripheral protocol inspectors.

The main debugger operates as a non-modal window toggled via the **Debugger** button in the Control Window. It features 10 specialized operational modes controlled by the [`Control::DebuggerMode`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Frontend/Control/UIModalState.h) state enumeration:

```
┌─────────────────┬─────────────────────────────────────────────────────────────┐
│ Mode (ID)       │ Module Functionality                                        │
├─────────────────┼─────────────────────────────────────────────────────────────┤
│ Settings (0)    │ Profiler and Telemetry Tuning & Sampling Controls           │
│ Disassembly (1) │ 65C02 CDL Disassembler & Hybrid Source Debugger (C / ASM)   │
│ Profiler (2)    │ Subroutine Call Stack, Flame Graph & Memory Access Heatmap  │
│ Debugger (3)    │ Interactive CPU Registers, 64KB Hex Editor & Memory Fuzzer  │
│ Breakpoints (4) │ Multi-Condition Boolean & Expression Breakpoint Manager     │
│ Memory Layout (5) 64 KB Memory Space Allocation & Device Conflict Map         │
│ Data Inspector (6) ASCII String Scanner, 16-bit Pointer & Signed Int Decoders │
│ Logic Analyzer (7) 4096-Sample Bus Waveforms, DIP IC Pinouts & Dual Cursors   │
│ Net Sniffer (8) │ ESP8266 AT Command & Network Packet Sniffer & ACIA Bridge   │
│ Memory Scanner(9) Cheat-Engine Multi-Pass Value Searching & Candidate Filter │
└─────────────────┴─────────────────────────────────────────────────────────────┘
```

---

## 2. Real-Time Disassembler & Symbol Table Support

The disassembler window ([`DisassemblerWindow.cpp`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Frontend/GUI/Debugger/DisassemblerWindow.cpp)) decodes memory instructions dynamically around the current Program Counter (`PC`).

```
┌──────┬────────┬──────────────┬──────────┬──────────────┬──────────────────────────────┐
│Label │Address │Bytes         │Mnemonic  │Operands      │Source Code                   │
├──────┼────────┼──────────────┼──────────┼──────────────┼──────────────────────────────┤
│      │        │              │          │              │// main.c:42                  │
│init  │$8000   │A9 00         │LDA       │#$00          │    uint8_t count = 0;        │
│      │$8002   │8D 00 02      │STA       │counter       │    counter = count;          │
│loop  │$8005   │EE 00 02      │INC       │counter       │    while(1) { counter++; }   │
│      │$8008   │4C 05 80      │JMP       │loop          │                               │
└──────┴────────┴──────────────┴──────────┴──────────────┴──────────────────────────────┘
```

### 2.1 Code-Data-Log (CDL) Hardware Tracking
Standard disassemblers often become desynchronized when encountering embedded lookup tables, graphics data, or jump addresses mixed within code segments. 

The bus hardware telemetry tracks memory execution semantics using the [`CDLFlags`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Hardware/Core/BusTelemetry.h) bitfield:
- `CDL_OPCODE (1 << 0)`: Address was fetched as an opcode by the CPU.
- `CDL_DATA (1 << 1)`: Address was accessed as data by an instruction.
- `CDL_ZP_POINTER (1 << 2)`: Address was read as a zero-page vector pointer.
- `CDL_OPERAND (1 << 3)`: Address was read as an instruction operand byte.

[`Hardware::Disassembler::DecodeInstruction()`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Hardware/CPU/Disassembler.cpp) checks:
```cpp
if (!isOpcode && isData) {
    inst.opcode = bus.ReadDirect(addr);
    inst.isDataByte = true; // Emitted as .byte $XX
    return inst;
}
```
If a memory address was read as data but never fetched as an instruction opcode, the disassembler treats it as `.byte $XX`. This prevents valid data tables from being misparsed as multi-byte opcodes.

### 2.2 Symbol Table Integration (`.dbg` & `.elf`)
[`Control::SymbolTable`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Hardware/Core/SymbolTable.h) loads symbol metadata from:
- **CC65 Debug Files (`.dbg`)**: Parsed via [`DbgFormatParser`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Hardware/Core/DbgFormatParser.h), loading modules, scopes, spans, line mapping, and symbols.
- **ELF/DWARF Binaries (`.elf`)**: Parsed via [`ElfSymbolExtractor`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Frontend/Compiler/ElfSymbolExtractor.h) and [`DwarfNmParser`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Hardware/Core/DwarfNmParser.h), extracting DWARF `.debug_line` tables and `nm` symbol dictionaries.

### 2.3 Hybrid C / Assembly View
When debug symbols are loaded, the disassembler can interleave high-level source code lines directly above their corresponding 6502 assembly output. Source lines are cached in [`SourceCodeCache`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Hardware/Core/SourceCodeCache.h), rendering the origin file and line number (e.g. `// kernel.c:118`).

### 2.4 Operand Symbol Resolution
Operand values are automatically translated to symbolic labels across all 65C02 addressing modes:
- `LDA $0200` $\rightarrow$ `LDA counter`
- `STA $0200,X` $\rightarrow$ `STA buffer,X`
- `JSR $8042` $\rightarrow$ `JSR init_display`
- `LDA ($20),Y` $\rightarrow$ `LDA (ptr),Y`

---

## 3. Subroutine Profiler & Flame Graph

The profiler suite ([`ProfilerWindow.cpp`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Frontend/GUI/Debugger/ProfilerWindow.cpp)) tracks function performance and memory hotspots without intrusive source code instrumenting.

### 3.1 Hierarchical Call Stack Profiling
[`Hardware::SubroutineProfiler`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Hardware/Core/SubroutineProfiler.h) hooks directly into instruction dispatch:
- **`JSR` (Jump to Subroutine)**: Pushes a new frame onto the internal call stack (up to 128 frames deep) recording entry cycle count.
- **`RTS` (Return from Subroutine)**: Pops the frame, computing total cycles spent inside the function and attributing child execution time.
- **`BRK` / `RTI`**: Tracks software breaks and interrupt service routines (`IRQ` / `NMI`).

### 3.2 Page Crossing Penalty Alerts
The 65C02 architecture incurs an additional clock cycle penalty when branch instructions or indexed addressing modes (`$1000,X`, `($20),Y`) cross a 256-byte memory page boundary.  
The [`Hardware::PageCrossTracker`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Hardware/Core/PageCrossTracker.h) detects:
$$\text{Page Cross} \iff (\text{Base Address} \ \& \ \text{\$FF00}) \neq (\text{Effective Address} \ \& \ \text{\$FF00})$$
The profiler flags these penalties under the **Page Cross Alerts** tab, identifying cycle losses in timing-critical loops.

### 3.3 Flame Graph Visualization
[`GUI::FlameGraphView`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Frontend/GUI/Debugger/FlameGraphView.h) renders an interactive visualization of the call hierarchy:
- **Box Width**: Proportional to the cumulative cycles spent in each subroutine relative to total CPU run time.
- **Box Color**: Distinct color hash derived from the function name or address.
- **Subroutine Metrics**: Displays total cycles, self cycles, call count, and page cross penalties upon hover.
- **Filtering**: Live search bar filters the visual tree by symbol name or address.

```
┌─────────────────────────────────────────────────────────────────────────────┐
│ [Flame Graph View]   Filter: [update________]                               │
├─────────────────────────────────────────────────────────────────────────────┤
│                               [update_physics] (12.4%)                      │
│                  [update_entities] (34.2%)    [render_sprites] (18.1%)      │
│ [main_game_loop] (78.5%)                                                    │
│ [root] (100.0%)                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 3.4 256×256 Memory Access Heatmap
Visualizes the access frequency across the entire 64 KB address space:
- **Coordinate Mapping**: A 256×256 pixel grid where $X = \text{address} \ \& \ \text{\$00FF}$ and $Y = \text{address} \gg 8$.
- **Logarithmic Normalization**: Memory hotspots (like Zero Page `$0000-$00FF` or Stack `$0100-$01FF`) often have millions of accesses that would otherwise saturate linear scales. The profiler applies:
  $$\text{Intensity} = \frac{\ln(\text{count} + 1)}{\ln(\text{max\_count} + 1)}$$
- **Channel Isolation**: Inspect Total Accesses, Read Counts (blue tint), or Write Counts (red tint).
- **Export Capabilities ([`MemoryHeatmapExporter`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Frontend/GUI/Debugger/MemoryHeatmapExporter.h))**:
  - **Export BMP**: Saves a high-resolution 256×256 uncompressed 24-bit bitmap with current logarithmic color mapping.
  - **Export RAW**: Saves a text file containing cycle-accurate access dumps (`0xXXXX: <reads> <writes>`).

---

## 4. CPU Register & Hex Memory Editor (`DebuggerMode::Debugger`)

The primary Debugger window ([`DebuggerWindow.cpp`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Frontend/GUI/Debugger/DebuggerWindow.cpp)) provides low-level interactive state inspection and live mutation for both CPU registers and raw memory.

### 4.1 Interactive CPU Registers
- **Real-Time Input Fields**: Direct hexadecimal inputs for `PC`, `SP`, `A`, `X`, and `Y`.
- **Safe Mutation Barrier**: Any edit made while the emulator is running automatically invokes `state.emulator.WaitUntilSafeToMutate()` to halt the core cleanly at an instruction boundary before applying changes.
- **Page Pointer Resynchronization**: If the Program Counter `PC` is altered, `cpu.UpdatePagePtr()` is immediately invoked against the bus to maintain instruction fetch consistency.

### 4.2 Virtualized 64KB Hex Memory Editor
- **Full Address Coverage**: Virtualized 16-byte row grid displaying addresses `$0000` through `$FFFF` via Dear ImGui's `ImGuiListClipper`.
- **Live In-Place Editing**: Click any byte cell to type new hexadecimal values directly. Writes are applied via `bus.WriteDirect(addr, val)` under mutation protection.
- **Dynamic Access Heatmap**:
  - Highlights active memory operations in real time: **Green** for current CPU `PC`, **Red** for recent memory writes, and **Blue** for recent memory reads.
  - While running, heatmap intensity decays exponentially over time (`bus.DecayHeatmap`), producing a visual phosphorescent trail of memory activity.

### 4.3 Memory Fuzzer & Fault Injection
Integrated directly into the bottom toolbar of the debugger view via [`Hardware::MemoryFuzzer`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Hardware/Core/MemoryFuzzer.h):
- **Fill Random Junk (`FuzzMemory`)**: Injects pseudo-random bytes across a user-specified memory range (`startAddr` to `endAddr`). Essential for testing that software properly clears uninitialized BSS segments, stacks, or variables.
- **Corrupt Random Byte (`CorruptRandomByte`)**: Selects a random RAM address and flips random bits or writes garbage data to test software resilience, error handling routines, and memory corruption tolerance.

---

## 5. Digital Logic Analyzer & IC Pinout Viewer

For hardware-level debugging, the simulator integrates a digital multichannel logic analyzer and interactive integrated circuit pinout viewer ([`LogicAnalyzerWindow.cpp`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Frontend/GUI/Debugger/LogicAnalyzerWindow.cpp)). The window features three operational view modes: **Waveforms** (digital tracks), **IC Pinout** (physical DIP package logic levels), and **Split View** (simultaneous waveforms and pinout display).

```
┌─────────────────────────────────────────────────────────────────────────────┐
│ Logic Analyzer  [Waveforms | IC Pinout | Split View]  Visible: [128 cycles] │
├─────────────────────────────────────────────────────────────────────────────┤
│ CLK     ─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐ │
│          └─┘ └─┘ └─┘ └─┘ └─┘ └─┘ └─┘ └─┘ └─┘ └─┘ └─┘ └─┘ └─┘ └─┘ └─┘ └─┘ │
│ ADDR   < $8000 >< $8001 >< $8002 >< $0200 >< $8005 >< $8006 >< $0200 >      │
│ DATA   <  $A9  ><  $00  ><  $8D  ><  $00  ><  $EE  ><  $00  ><  $01  >      │
│ R/W    ───────────────────────────┐       ─────────────────────┐     ────── │
│                                   └───────┘                     └─────┘     │
│ SYNC   ─────────┐                 ──────────────────┐                       │
│                 └─────────────────┘                 └────────────────────── │
│ RDY    ──────────────────────────────────────────────────────────────────── │
│ /IRQ   ──────────────────────────────────────────────────────────────────── │
├─────────────────────────────────────────────────────────────────────────────┤
│ Cursor A: Cycle #480102  Cursor B: Cycle #480118  Delta: 16 cycles (16.00 µs)│
└─────────────────────────────────────────────────────────────────────────────┘
```

### 5.1 Logic Analyzer Architecture
[`Hardware::LogicAnalyzer`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Hardware/Core/LogicAnalyzer.h) records a continuous 4096-sample ring buffer of [`LogicSample`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Hardware/Core/LogicAnalyzer.h) structures:

```cpp
struct LogicSample {
    uint64_t cycle = 0;       // Monotonic CPU cycle counter
    uint16_t address = 0;     // 16-bit address bus lines A0-A15
    uint8_t data = 0;         // 8-bit data bus lines D0-D7
    uint8_t rw : 1 = 1;       // R/W line (1 = Read, 0 = Write)
    uint8_t sync : 1 = 0;     // SYNC line (1 during opcode fetch)
    uint8_t rdy : 1 = 1;      // RDY line (Wait-state control)
    uint8_t irq : 1 = 0;      // Active-low /IRQ status
    uint8_t nmi : 1 = 0;      // Active-low /NMI status
    uint8_t ca1 : 1, ca2 : 1; // VIA Port A control lines
    uint8_t cb1 : 1, cb2 : 1; // VIA Port B control lines
    uint8_t spiCs : 1, spiClk : 1, spiMosi : 1, spiMiso : 1; // SD SPI Bus
    uint8_t uartTx : 1, uartRx : 1;                          // ACIA UART
    uint8_t portA = 0;        // VIA 8-bit Port A lines PA0-PA7
    uint8_t portB = 0;        // VIA 8-bit Port B lines PB0-PB7
};
```

### 5.2 Waveform Navigation & Dual Timing Cursors
- **Zoom & Pan**: Adjust visible timebases from 16 up to 512 cycles per window.
- **Port Bit Expansion**: Toggle `showPortBits` to unpack Port A and Port B into individual digital tracks (`PA0-PA7`, `PB0-PB7`).
- **Dual Cursors (Cursor A & Cursor B)**: Draggable timing cursors with real-time computation of delta clock cycles and real-world time ($\Delta t = \frac{\Delta \text{cycles}}{\text{IPS}}$), formatted automatically in $\text{ns}$, $\mu\text{s}$, $\text{ms}$, or $\text{s}$.

### 5.3 Interactive IC Pinout Viewer ([`ICPinoutViewer.cpp`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Frontend/GUI/Debugger/ICPinoutViewer.cpp))
Simulates the physical DIP (Dual In-line Package) logic state at the current cycle:

1. **W65C02S CPU (40-Pin PDIP)**:
   - Visualizes address pins ($A_0-A_{15}$), data bus ($D_0-D_7$), clock outputs ($\Phi_1, \Phi_2$), and control pins (`/VPB`, `RDY`, `/IRQB`, `/MLB`, `/NMIB`, `SYNC`, `VDD`, `VSS`).
2. **W65C22 VIA (40-Pin PDIP)**:
   - Visualizes bidirectional Port A ($PA_0-PA_7$) and Port B ($PB_0-PB_7$), handshake control lines ($CA_1, CA_2, CB_1, CB_2$), register select pins ($RS_0-RS_3$), and chip selects ($CS_1, /CS_2$).
3. **W65C51N ACIA (28-Pin PDIP)**:
   - Visualizes serial UART transmission lines (`TxD`, `RxD`), modem handshake controls (`/RTS`, `/CTS`, `/DTR`, `/DSR`, `/DCD`), register selects, and baud clocks.
- **Logic Level Colors**: High / 5V (Green), Low / 0V (Dark Gray), Active-Low asserted (Red), Power / VDD / VSS (Neutral Gray).

---

## 6. Memory Scanner (Cheat Engine)

The Memory Scanner ([`MemoryScannerWindow.cpp`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Frontend/GUI/Debugger/MemoryScannerWindow.cpp) & [`MemoryScanner.h`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Hardware/Core/MemoryScanner.h)) allows developers to locate variable locations in RAM during execution.

### 6.1 Scanning Capabilities
- **Value Types**:
  - `8-bit (U8)`: Unsigned 8-bit integer ($0-255$).
  - `16-bit LE (U16)`: Little-endian 16-bit integer ($0-65535$).
  - `String`: Sequential ASCII character strings.
- **Scan Filters**:
  - `Exact`: Value equals target.
  - `Increased`: Value has increased compared to previous scan.
  - `Decreased`: Value has decreased compared to previous scan.
  - `Unchanged`: Value has remained identical to previous scan.
  - `Unknown Initial Value`: Captures all memory addresses in the range during the first scan.

### 6.2 Multi-Pass Workflow
1. Select target memory range (default: `$0000-$FFFF` or RAM `$0000-$7FFF`).
2. Click **First Scan** to initialize candidate list.
3. Advance the emulation or perform an in-game action (e.g. lose a life, score points).
4. Select comparison filter (`Increased`, `Decreased`, or new `Exact` value) and click **Next Scan**.
5. The candidates list refines until the exact memory address is isolated. Changes in current values are highlighted in yellow.
6. Once isolated, the developer can inspect or mutate the address in the Memory Editor.

---

## 7. Data Inspector & Format Decoders

The Data Inspector ([`DataInspectorWindow.cpp`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Frontend/GUI/Debugger/DataInspectorWindow.cpp)) decodes memory regions into multiple data formats simultaneously.

### 7.1 Presets & Device Selections
One-click presets target common architectural structures:
- **Zero Page**: `$0000-$00FF` (fast direct-page variables).
- **Stack**: `$0100-$01FF` (hardware return addresses and pushed registers).
- **Vectors**: `$FFFA-$FFFF` (NMI, RESET, IRQ/BRK hardware vectors).
- **Bus Device Presets**: Automatically adjusts boundaries to match any registered peripheral (`validDevices`: VIA, ACIA, SID, GPU VRAM).

### 7.2 Inspection Tabs
1. **ASCII Strings Tab ([`MemoryDataFormatter::DetectAsciiStrings`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Frontend/GUI/Debugger/MemoryDataFormatter.h))**:
   - Scans the selected range for null-terminated or printable ASCII text sequences with configurable minimum length (default: 3 characters).
   - Displays offset address, string length, and string text.
2. **16-bit Pointers Tab**:
   - Parses sequential word pairs as little-endian addresses:
     $$\text{Target} = \text{bus}[a] \ | \ (\text{bus}[a+1] \ll 8)$$
   - Resolves whether the pointed address falls into RAM, ROM, or MMIO, and cross-references symbols from the `SymbolTable`.
3. **Signed Integers Tab**:
   - Decodes values simultaneously as signed 8-bit ($-128 \dots +127$) and signed 16-bit two's complement integers.
4. **C-Array Exporter**:
   - Formats any memory range into copy-pasteable C/C++ array declarations (`const uint8_t data[] = { 0x00, ... };`).

---

## 8. ESP8266 Network Traffic Sniffer

The Network Sniffer ([`NetworkSnifferWindow.cpp`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Frontend/GUI/Debugger/NetworkSnifferWindow.cpp)) monitors serial and IP communications handled by the virtual ESP8266 WiFi peripheral.

### 8.1 Traffic Direction Badges
- `TX` (Accent Primary / Cyan): Commands sent from 65C02 to ESP8266 over the bus.
- `RX` (Accent Active / Blue): Responses returned from ESP8266 to 65C02.
- `NET TX` (Orange): Outbound socket payloads transmitted over host TCP/IP sockets.
- `NET RX` (Purple): Inbound network packets received from remote web servers or TLS sockets.

### 8.2 Traffic Channels
- `ATCommand`: AT command strings (`AT+CWJAP`, `AT+CIPSTART`, `AT+CIPSEND`).
- `ATResponse`: Peripheral response codes (`OK`, `ERROR`, `CONNECT`).
- `DataPayload`: Raw binary network payload buffers.
- `TLS_HTTP`: Outbound HTTPS / TLS negotiation packets.
- `System`: WiFi module link-state notifications and DHCP events.

### 8.3 Packet Inspection & Bridge Mode
- **Hex + ASCII Dump**: Selecting any packet displays a dual hexadecimal and ASCII inspection pane.
- **ACIA Bridge Toggle**: Bridges ESP8266 UART directly to the ACIA terminal console for manual interactive AT command debugging.

---

## 9. Memory Layout & Conflict Viewer

The Memory Layout window ([`MemoryLayoutWindow.cpp`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Frontend/GUI/Debugger/MemoryLayoutWindow.cpp)) renders a 256×256 texture representing the entire 64 KB memory map.

- **Color-Coded Allocations**: Every registered bus device is rendered in a distinctive color according to theme conventions (RAM, ROM, VIA, ACIA, SID, GPU, SD Card).
- **Overlap & Conflict Highlighting**: If two devices register overlapping addresses (for instance, an incorrectly mapped MMIO peripheral conflicting with RAM), the intersecting area is blended in contrasting warning colors.
- **Device Details**: Displays base start address, end address, total capacity in bytes, enabled status, and bus priority.
