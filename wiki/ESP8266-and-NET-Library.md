# ESP8266 and NET.h Library

**Directory:** `Hardware/Comm/` and `Binaries/Libs/`

## Overview

The ESP8266 is a virtual hardware component added to the 65c02 Simulator to provide networking capabilities, primarily through Wi-Fi and TCP/IP, using standard AT commands. The `NET.h` library provides a simple C interface to communicate with this virtual ESP8266 module from your 65c02 programs.

## Memory Mapping

The ESP8266 is dynamically mapped into the system's memory when the `NET.h` library is included in a C program. The memory is mapped just after the ACIA, occupying 4 bytes:

- **Address Range:** `0x5004` – `0x5007`

When you compile a program that includes `#include "Libs/NET.h"`, the compilation script `compile-bin.sh` notices this and flags the `generate_cfg.py` script to reserve this memory space in the dynamic linker configuration (`C-Runtime-dynamic.cfg`).

### Registers

The virtual ESP8266 provides 4 registers in the memory:

| Address | Name | Description |
|---------|------|-------------|
| `0x5004` | `DATA` | **Read/Write:** Used to send bytes to the ESP8266 and read bytes received. |
| `0x5005` | `STATUS` | **Read:** Status bits. Bit 7 is set (`0x80`) if there is data available to read in the RX queue. |
| `0x5006` | `CMD` | **Read/Write:** Command register for special internal commands. (Not mostly used by end users directly) |
| `0x5007` | `CTRL` | **Read/Write:** Control register. |

## NET.h Library

The C library provides simple helper macros and functions to interact with the module without manually handling memory addresses.

### Available Functions & Macros

#### `net_has_data()`
Returns true (`1`) if the ESP8266 has data ready to be read. It checks bit 7 of the `STATUS` register.

#### `net_write_raw_byte(b)`
Writes a single raw byte directly to the `DATA` register.

#### `net_read_raw_byte()`
Reads a single raw byte directly from the `DATA` register. You should check `net_has_data()` before reading.

#### `net_send_string(const char* str)`
Sends a null-terminated string to the ESP8266 character by character.

#### `net_send_cmd(const char* cmd)`
Sends an AT command as a string, automatically appending `\r\n` (Carriage Return + Line Feed) at the end, which is required by the ESP8266 to process AT commands.

## Underlying Implementation (`ESP8266.cpp`)

In the emulator code (`Hardware/Core/ESP8266.cpp`), the component implements basic AT command handling, background thread polling for TCP connections using `asio` (or similar network libraries), and a queue mechanism to feed bytes back to the 6502 at its own pace. 
