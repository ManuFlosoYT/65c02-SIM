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

## Integrity and Validation (SHA-256)

To protect against corrupted, truncated, or incompatible save state files, the emulator implements a strict integrity validation mechanism.

1.  **Magic Header**: The save file always begins with the static magic sequence `"SIM65C02SST"`.
2.  **Payload Data**: The raw binary data representing the emulator state payload.
3.  **Hash Verification**: During serialization, the emulator uses the **PicoSHA2** library to compute a **SHA-256** cryptographic hash representation of the payload. This 64-character hexadecimal hash string is appended to the very end of the save file.

When you load a state file (via **Settings -> Load State**), the emulator recalculates the hash of the data it reads and compares it directly against the hash signature at the end of the file.

If the file has been modified externally, downloaded incompletely, or corrupted in any way, the calculated hash will diverge, and the emulator will refuse to load the state, preventing potentially dangerous undefined behavior or crashes.

### Ignoring Hash Validation

If you intentionally modify the internal bits of a save state for memory hacking or debugging purposes, you can forcefully bypass the security check.
Simply toggle on the **"Ignore Save State Hash"** option in the **Settings Menu** before clicking **Load State**.
