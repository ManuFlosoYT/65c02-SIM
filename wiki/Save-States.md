# Save States

The 65c02-SIM emulator supports saving and loading the complete state of the machine to and from binary files. This feature is particularly useful for debugging, pausing long computations, or keeping track of game progress.

## Overview

When you save the state (via **Settings -> Save State**), the emulator pauses execution and serializes the exact instantaneous state of every single hardware component into a binary `.state` file.

The components serialized include:

- **Memory (RAM & ROM)**: The entire 64KB memory space is perfectly preserved.
- **CPU**: All registers (A, X, Y, PC, SP), flags, and execution cycle state.
- **VIA**: Internal timers, shift registers, and port states.
- **SID**: Oscillator settings, envelope states, and volume.
- **ACIA**: Data, status, command, and control registers.
- **LCD Controller**: Display RAM, cursor position, and controller modes.
- **GPU**: Pixel coordinates and rendering state (VRAM is restored automatically from main memory to save 7.5KB of space).
- **Console Window**: The current terminal history and cursor position.

The save operation happens synchronously with the `emulationMutex` locked, making it completely thread-safe and guaranteeing absolute consistency across components.

## Integrity and Validation (Versioned)

To protect against corrupted, truncated, or incompatible save state files, the emulator implements a strict integrity validation mechanism.

1.  **Magic Header**: The save file always begins with the static magic sequence `"SIM65C02SST"`.
2.  **Metadata Layer**:
    - **Build Version**: The `PROJECT_VERSION` (git tag/commit) is embedded at the start of the payload.
    - **Memory Map**: A snapshot of the registered hardware devices and their address ranges.
3.  **Hash Verification**: During serialization, the emulator computes a **SHA-256** cryptographic hash of the payload. This 64-character hexadecimal hash string is appended to the very end of the file.

When loading:

1. The emulator checks the hash.
2. It attempts to load all components even if there is a version mismatch (printing a warning).
3. If the load fails and a version mismatch was detected, it reports an incompatibility error.

### Force Load Savestate

If you intentionally modify the internal bits of a save state or wish to ignore version/hash warnings, you can use the **"Force load savestate"** option in the **Settings Menu**.
When enabled, the emulator will ignore hash mismatches, version discrepancies, and stream errors, attempting to restore the machine state as completely as possible.
