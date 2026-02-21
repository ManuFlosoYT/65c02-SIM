# 65c02-SIM Wiki

Welcome to the wiki for the **65c02 emulator** for Ben Eater's memory layout.

## What is this project?

This project is a full emulator of the **WDC 65C02** microprocessor (the CMOS variant of the classic MOS Technology 6502), designed around the memory layout used in [Ben Eater's](https://eater.net/6502) 65c02 projects. It includes a modern **ImGui**-based graphical interface and a complete SDK toolchain for developing software that runs on the emulator.

## Emulated components

| Component | Description |
|-----------|-------------|
| **CPU** | Full WDC 65C02 processor |
| **RAM** | 32 KB of random-access memory (`0x0000`–`0x7FFF`) |
| **ROM** | 32 KB of read-only memory (`0x8000`–`0xFFFF`) |
| **VIA** | MOS 6522 Versatile Interface Adapter (timers, I/O) |
| **ACIA** | MOS 6551 Asynchronous Communications Interface (serial) |
| **GPU** | Custom graphics controller with 7.5 KB of VRAM |
| **SID** | Sound synthesis chip (3 oscillators + ADSR) |
| **LCD** | 2×16 character LCD display |

## General architecture

```
┌──────────────────────────────────────────────────────┐
│              SIM_65C02 (Main Executable)             │
├──────────────────────────────────────────────────────┤
│                  Frontend / GUI                      │
│     ImGui · SDL3 · OpenGL 3.3 · ImGuiFileDialog      │
├──────────────────────────────────────────────────────┤
│                   Control Layer                      │
│           AppState · Console · UpdateChecker         │
├──────────────────────────────────────────────────────┤
│           65c02_core Static Library                  │
│  ┌───────────┬────────────┬──────────────────────┐   │
│  │  CPU      │  Memory    │  Peripherals         │   │
│  │ (65c02)   │ (RAM+ROM)  │  VIA · ACIA · LCD    │   │
│  ├───────────┼────────────┼──────────────────────┤   │
│  │     GPU (VRAM 7.5 KB)  │  SID (synthesis)     │   │
│  └───────────┴────────────┴──────────────────────┘   │
├──────────────────────────────────────────────────────┤
│               SDK / Tools                            │
│   compile-bin.sh · image-to-bin.sh · midi-to-bin.sh  │
│     Linker configs · BIOS · Microsoft BASIC          │
├──────────────────────────────────────────────────────┤
│            Example Programs & Tests                  │
│    Binaries/ (C + ASM)  ·  UnitTests/ (Google Test)  │
└──────────────────────────────────────────────────────┘
```

## Wiki page map

- **Hardware & emulation**
  - [Hardware](Hardware) — Main emulator orchestrator and memory system
  - [CPU](CPU) — 65c02 processor and registers
  - [CPU Instructions](CPU-Instructions) — Full instruction set reference
  - [GPU](GPU) — Graphics processor and VRAM
  - [SID](SID) — Sound synthesis
  - [VIA and ACIA](VIA-and-ACIA) — I/O chips
  - [LCD](LCD) — LCD display
- **User interface**
  - [Frontend](Frontend) — ImGui windows and controls
- **Development & SDK**
  - [Linker and Memory Layout](Linker-and-Memory-Layout) — Linker configs, BIOS, memory map
  - [SDK](SDK) — Compilation and conversion tools
  - [Binaries](Binaries) — Included example programs
- **Building & testing**
  - [Building](Building) — Compile the project from source
  - [Unit Tests](Unit-Tests) — Unit test infrastructure

## Quick start

### Run the simulator (pre-compiled binaries)

1. Download the latest release from the [releases page](https://github.com/ManuFlosoYT/65c02-SIM/releases).
2. Also download `SDK.zip` if you want to develop programs.
3. Open the simulator and select the `.bin` file you want to run.

### Compile a program from the SDK

```bash
# Compile a C or assembly program
./compile-bin.sh <program_name>

# Convert an image to VRAM format
./image-to-bin.sh <image>

# Convert a MIDI file to SID assembly code
./midi-to-bin.sh <midi_file>
```

See the [SDK](SDK) page for more details.

## License

The emulator C++ source code is licensed under the **MIT License**.  
The project includes Microsoft BASIC (© 1977 Microsoft) and WozMon (© 1976 Apple Computer, Inc.) for educational purposes.
