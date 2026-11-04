# SD Card & FAT16 Storage Emulation

**Source Files:**
- Core SD Emulation: [`src/Hardware/Storage/SDCard.h`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Hardware/Storage/SDCard.h) / [`src/Hardware/Storage/SDCard.cpp`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Hardware/Storage/SDCard.cpp)
- Virtual FAT16 Synchronizer: [`src/Hardware/Storage/VirtualFAT16Synchronizer.h`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Hardware/Storage/VirtualFAT16Synchronizer.h) / [`src/Hardware/Storage/VirtualFAT16Synchronizer.cpp`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Hardware/Storage/VirtualFAT16Synchronizer.cpp)
- FAT16 Image Utilities: [`src/Hardware/Storage/FAT16Utils.h`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Hardware/Storage/FAT16Utils.h)
- SDK Drivers: [`sdk/src/Libs/SD.h`](file:///home/manu/Documents/GitHub/65c02-SIM/sdk/src/Libs/SD.h), [`sdk/src/Libs/fatfs/diskio.c`](file:///home/manu/Documents/GitHub/65c02-SIM/sdk/src/Libs/fatfs/diskio.c), [`sdk/microdos/Libs/app-bios.h`](file:///home/manu/Documents/GitHub/65c02-SIM/sdk/microdos/Libs/app-bios.h)
- Test Suite: [`src/UnitTests/SDCard_Test.cpp`](file:///home/manu/Documents/GitHub/65c02-SIM/src/UnitTests/SDCard_Test.cpp)

---

## 1. Overview

The 65c02-SIM storage subsystem emulates a high-performance **Secure Digital (SD) card interface** operating over the **Serial Peripheral Interface (SPI Mode 0)**. Rather than requiring software bit-banging across general-purpose VIA I/O pins, the emulator implements a dedicated hardware **Memory-Mapped I/O (MMIO) SPI Host Controller**.

The controller supports both **SDSC** (Standard Capacity, byte-addressed) and **SDHC** (High Capacity, block-addressed) command protocols, fixed 512-byte sector transfers, CRC validation, read-latency simulation, and bi-directional folder synchronization with the host filesystem via the **Virtual FAT16 Synchronizer**.

---

## 2. MMIO Register Map

The SD Card interface occupies a 4-byte window. The physical base address depends on whether the system runs in standard mode or under a microDOS / Cartridge layout:

| Address (Default System) | Address (microDOS Cartridge) | Address (Custom Cartridge SPI) | Register | Access | Hardware Function |
|:---:|:---:|:---:|:---|:---:|:---|
| **`0x5008`** | **`0x7FC8`** | **`0x7FC0`** | `SD_CTRL` | R/W | **Read:** Bit 0 indicates mount status (`1` = Card mounted, `0` = Unmounted).<br>**Write:** Bit 0 controls Chip Select (`1` = Assert /CS Low, `0` = Deassert /CS High). |
| **`0x5009`** | **`0x7FC9`** | **`0x7FC1`** | `SD_DATA` | R/W | **Write:** Initiates an 8-bit SPI transmission (MOSI).<br>**Read:** Returns the last byte shifted in from the card (MISO). |
| `0x500A` | `0x7FCA` | `0x7FC2` | *Reserved* | R | Returns `0xFF`. |
| `0x500B` | `0x7FCB` | `0x7FC3` | *Reserved* | R | Returns `0xFF`. |

### 2.1 Chip Select Control Logic

The SD card uses an **active-low Chip Select (`/CS`)** line. In the MMIO interface:

- Writing `1` to `SD_CTRL` (`data & 0x01 != 0`) drives `/CS` **LOW** (card selected).
- Writing `0` to `SD_CTRL` drives `/CS` **HIGH** (card deselected).
- Deselecting `/CS` while idle resets command parsing buffers and aborts uncommitted transfers.
- Reading `SD_CTRL` returns `0x01` if an image or folder is currently mounted, and `0x00` if the slot is empty.

### 2.2 SPI Data Shifting

The hardware controller automatically clocks 8 SPI cycles upon any write to `SD_DATA`:

```c
// spi_xfer logic (diskio.c)
static uint8_t spi_xfer(uint8_t mosi) {
    SD_DATA = mosi; // Writing byte initiates transmission
    return SD_DATA; // Reading register immediately yields received MISO
}
```

---

## 3. SD Command State Machine & Protocol

The internal state machine ([`SDCard.cpp`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Hardware/Storage/SDCard.cpp)) processes standard 6-byte SPI command frames:

```
Byte 0: [ 0 1 ] [ 6-bit Command Index ]
Byte 1: [ 32-bit Argument (MSB)        ]
Byte 2: [ 32-bit Argument              ]
Byte 3: [ 32-bit Argument              ]
Byte 4: [ 32-bit Argument (LSB)        ]
Byte 5: [ 7-bit CRC ] [ Stop Bit: 1    ]
```

### 3.1 Supported SD Commands

| Command | Name | Arg | Response | Hardware Action / Behavior |
|:---|:---|:---:|:---:|:---|
| **`CMD0`** | `GO_IDLE_STATE` | `0` | **R1** (`0x01`) | Enters SPI Mode, resets controller state, returns In-Idle state. |
| **`CMD8`** | `SEND_IF_COND` | `0x1AA` | **R7** | Checks voltage compatibility. Emits echo pattern `0xAA` validating SDv2+ support. |
| **`CMD12`** | `STOP_TRANSMISSION` | `0` | **R1b** | Aborts multi-block reads/writes. Holds MISO low until the busy state clears. |
| **`CMD13`** | `SEND_STATUS` | `0` | **R2** | Returns 16-bit card operational status. |
| **`CMD16`** | `SET_BLOCKLEN` | `512` | **R1** | Enforces block length (fixed at 512 bytes). |
| **`CMD17`** | `READ_SINGLE_BLOCK` | LBA | **R1** + Data | Reads single 512-byte sector preceded by token `0xFE` and followed by 2-byte CRC16. |
| **`CMD18`** | `READ_MULTIPLE_BLOCK`| LBA | **R1** + Stream | Streams consecutive 512-byte sectors until terminated by `CMD12`. |
| **`CMD24`** | `WRITE_SINGLE_BLOCK` | LBA | **R1** + Token | Receives token `0xFE`, writes 512 bytes, verifies CRC16, emits R1b busy response. |
| **`CMD25`** | `WRITE_MULTIPLE_BLOCK`| LBA | **R1** + Stream | Receives consecutive blocks preceded by token `0xFC` until Stop Token `0xFD` is received. |
| **`CMD55`** | `APP_CMD` | `0` | **R1** | Flags subsequent command as an Application Specific Command (`ACMD`). |
| **`CMD58`** | `READ_OCR` | `0` | **R3** | Returns Operation Conditions Register. Bit 30 set confirms SDHC / block addressing. |
| **`CMD59`** | `CRC_ON_OFF` | `0`/`1`| **R1** | Enables or disables CRC7 command and CRC16 data payload verification. |
| **`ACMD41`**| `SD_SEND_OP_COND` | HCS bit | **R1** (`0x00`) | Initializes card. Polls until internal initialization completes (`0x00` returned). |

### 3.2 Response Types

- **R1:** 1 byte (`0x00` = OK, `0x01` = In Idle State, `0x04` = Illegal Command).
- **R1b:** R1 followed by busy line holding MISO `0x00` until internal write/erase completes.
- **R2:** 2 bytes (R1 + status byte).
- **R3:** 5 bytes (R1 + 32-bit OCR register).
- **R7:** 5 bytes (R1 + 32-bit interface condition: echo back of supply voltage and check pattern).

### 3.3 Realistic Read Latency Emulation

When `read_latency_enabled` is active, the virtual SD card does not return data tokens instantaneously. Instead, the card emits a configurable number of padding bytes (`0xFF`) simulating the seek and read latency of flash memory before asserting the `0xFE` start block token. This ensures drivers correctly implement polling loops.

---

## 4. Virtual FAT16 Synchronizer

The emulator provides live, bi-directional folder synchronization between the host operating system and the emulated 65c02 disk through [`VirtualFAT16Synchronizer`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Hardware/Storage/VirtualFAT16Synchronizer.h):

```
+--------------------------+                         +--------------------------+
|       Host Folder        |  ─── SyncFromHost() ──> |    Virtual FAT16 Disk    |
|   (Linux / macOS / Win)  |  <───  SyncToHost() ─── |    (32 MB RAM Vector)    |
+--------------------------+                         +--------------------------+
```

### 4.1 Master Boot Record (MBR) & BPB Parameters

The in-memory filesystem generates an authentic FAT16 disk geometry:

| BIOS Parameter Block (BPB) Field | Offset | Value | Description |
|:---|:---:|:---:|:---|
| `BS_OEMName` | `0x03` | `"6502SIM "` | OEM identifier string |
| `BPB_BytsPerSec` | `0x0B` | `512` (`0x0200`) | Standard sector size |
| `BPB_SecPerClus` | `0x0D` | `8` | 4,096 bytes per cluster ($8 \times 512$) |
| `BPB_RsvdSecCnt` | `0x0E` | `4` | 4 reserved sectors before FAT 1 |
| `BPB_NumFATs` | `0x10` | `2` | Redundant File Allocation Tables (FAT 1 & FAT 2) |
| `BPB_RootEntCnt` | `0x11` | `512` (`0x0200`) | 512 root directory entries ($512 \times 32\text{ bytes} = 16\text{ KB}$) |
| `BPB_TotSec16` | `0x13` | `0` | Overridden by 32-bit total sector count |
| `BPB_Media` | `0x15` | `0xF8` | Fixed disk media descriptor |
| `BPB_FATSz16` | `0x16` | `32` (`0x0020`) | 32 sectors per FAT ($32 \times 512 = 16\text{ KB}$) |
| `BPB_TotSec32` | `0x20` | `65536` (`0x00010000`) | Total volume size = **32 Megabytes** |
| `BS_BootSig` | `0x1FE` | `0x55, 0xAA` | Valid bootable sector signature |

### 4.2 Disk Layout Map

$$\begin{aligned}
\text{Reserved Area} &= \text{Sectors } 0 \dots 3 \quad (4\text{ sectors}) \\
\text{FAT 1} &= \text{Sectors } 4 \dots 35 \quad (32\text{ sectors}) \\
\text{FAT 2} &= \text{Sectors } 36 \dots 67 \quad (32\text{ sectors}) \\
\text{Root Directory} &= \text{Sectors } 68 \dots 99 \quad (32\text{ sectors}) \\
\text{Data Area (Cluster 2+)} &= \text{Sectors } 100 \dots 65535 \quad (65,436\text{ sectors})
\end{aligned}$$

### 4.3 SFN Generation & Name Collisions

Since FAT16 natively supports only 8.3 Short File Names (SFN), `VirtualFAT16Synchronizer::GenerateSFN()` transforms modern filenames into uppercase 8.3 representations:
1. Strips invalid characters, spaces, and duplicate periods.
2. Formats names fitting $8 + 3$ characters directly (`README.TXT`).
3. For longer filenames or collisions, appends a numeric tail (`README~1.TXT`, `README~2.TXT`).
4. Maintains an internal mapping map (`sfnToHostMap`) so modifications made inside the simulator sync back to the original host filename!

### 4.4 Host Deletion Safeguards

To prevent catastrophic data loss when synchronization runs:
- `IsProtectedHostPath()` strictly prohibits deletion or syncing into root directories, `/home`, `/usr`, `/etc`, `C:\Windows`, `Program Files`, or user profile roots.
- Host deletions must be explicitly enabled via `allow_host_deletion`.

---

## 5. Cartridge System Integration

Cartridges (`.cart`) can embed an SD card image directly within the package or link to an external image file:

```json
{
  "system": {
    "sdEnabled": true,
    "sdCardPath": "disk.img"
  },
  "devices": [
    { "name": "SD Card", "start": "0x7FC8", "end": "0x7FCB" }
  ]
}
```

If a binary is loaded without a cartridge, the simulator automatically searches for a sibling file with the extension `.sd` (e.g. `program.bin` loads `program.sd`) and mounts it on startup.

---

## 6. Software Drivers & Programming Examples

### 6.1 65c02 Assembly: Low-Level Sector Read

```assembly
; =====================================================================
; SD_ReadSector: Reads 512-byte block at LBA in $02-$05 into ($00,$01)
; MMIO Registers: SD_CTRL = $5008, SD_DATA = $5009
; =====================================================================
SD_CTRL = $5008
SD_DATA = $5009

ReadSector:
    ; 1. Assert Chip Select (/CS Low)
    LDA #$01
    STA SD_CTRL

    ; 2. Transmit CMD17 ($51)
    LDA #$51
    STA SD_DATA
    LDA SD_DATA         ; Discard response

    ; 3. Transmit 32-bit LBA (Big-Endian)
    LDA $05
    STA SD_DATA
    LDA $04
    STA SD_DATA
    LDA $03
    STA SD_DATA
    LDA $02
    STA SD_DATA

    ; 4. Transmit Dummy CRC byte
    LDA #$FF
    STA SD_DATA

    ; 5. Wait for R1 Response (Poll for 0x00)
@WaitR1:
    LDA #$FF
    STA SD_DATA
    LDA SD_DATA
    CMP #$00
    BNE @WaitR1

    ; 6. Wait for Data Token (0xFE)
@WaitToken:
    LDA #$FF
    STA SD_DATA
    LDA SD_DATA
    CMP #$FE
    BNE @WaitToken

    ; 7. Read 512 Data Bytes
    LDY #0
@ReadBlock1:
    LDA #$FF
    STA SD_DATA
    LDA SD_DATA
    STA ($00),Y
    INY
    BNE @ReadBlock1

    INC $01             ; Next 256-byte page
@ReadBlock2:
    LDA #$FF
    STA SD_DATA
    LDA SD_DATA
    STA ($00),Y
    INY
    BNE @ReadBlock2

    ; 8. Read 2-byte CRC16 (discard)
    LDA #$FF
    STA SD_DATA
    LDA #$FF
    STA SD_DATA

    ; 9. Deassert Chip Select (/CS High)
    LDA #$00
    STA SD_CTRL
    RTS
```

### 6.2 C Language: Reading Files via FatFs (`app-bios.h`)

```c
#include <stdint.h>
#include <stdio.h>
#include "Libs/app-bios.h"

void read_file_example(const char* filename) {
    SD_FILE file;
    char buffer[64];
    int16_t bytesRead;

    // Open file in read mode
    if (sd_open(&file, filename, SD_READ) != 0) {
        println("Error opening file!");
        return;
    }

    // Read blocks until EOF
    while ((bytesRead = sd_read(&file, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytesRead] = '\0';
        print_str(buffer);
    }

    // Close handle
    sd_close(&file);
    println("\nFile read complete.");
}
```
