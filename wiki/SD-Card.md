# SD Card Emulation

The emulator includes a high-level emulation of an SD Card interface using a dedicated **Memory Mapped I/O (MMIO)** SPI controller. This allows programs to store and retrieve data from a virtual SD card image file (FAT16 format) using the standard **SPI (Serial Peripheral Interface)** protocol.

The system emulates the behavior of **SDSC** (Standard Capacity) and **SDHC** (High Capacity) cards.

## Technical Specifications

- **Emulation Type**: SPI Mode 0 (CPOL=0, CPHA=0).
- **Card Type**: SDSC (Standard Capacity) and SDHC (High Capacity) protocols supported.
- **Block Size**: Fixed at 512 bytes.
- **Supported Commands**:
  - `CMD0` (GO_IDLE_STATE)
  - `CMD8` (SEND_IF_COND)
  - `CMD16` (SET_BLOCKLEN)
  - `CMD17` (READ_SINGLE_BLOCK)
  - `CMD24` (WRITE_SINGLE_BLOCK)
  - `CMD55` (APP_CMD)
  - `CMD58` (READ_OCR)
  - `ACMD41` (SD_SEND_OP_COND)

## Memory Mapped I/O Interface

The SD Card hardware is directly memory-mapped, providing a much higher performance interface compared to the previous bit-banging method. It occupies the address range `0x5008`–`0x500B`.

### Registers

| Address | Name | Access | Description |
| ------- | ---- | ------ | ----------- |
| **0x5008** | **SD_CTRL** | R/W | **Read:** Bit 0: 1 if card is mounted, 0 otherwise.<br>**Write:** Bit 0: 1 to select card (/CS Low), 0 to deselect (/CS High). |
| **0x5009** | **SD_DATA** | R/W | **Write:** Initiates an SPI transfer (MOSI).<br>**Read:** Returns the last byte received from the card (MISO). |

> [!NOTE]
> Unlike the old bit-banging interface, this dedicated controller handles the clocking and bit-shifting automatically. Writing a byte to `SD_DATA` immediately completes a full 8-bit SPI transfer.

## Enabling the Hardware

By default, the SD Card hardware is **disabled** in the configuration. To use the SD Card:

1.  Open the **Settings** window (accessible from the Debugger menu).
2.  In the **Storage (SD Card)** section, select or create an `.img` file.
3.  Ensure the SD Card is enabled in the **Memory Layout** (where it will appear at its dedicated `0x5008` address).

The emulator will automatically mount the image if a file with the same name as the ROM but with `.sd` extension exists in the same folder.

> [!NOTE]
> Try running the `testSD` program from the SDK to test the SD Card functionality.

## Software Support

### SDK Library
The project's SDK includes a high-level wrapper for the FatFs library. You can use it by including `Libs/SD.h` in your C programs. It provides standard functions like:
- `sd_mount()`
- `sd_ls()`
- `sd_open()` / `sd_read()` / `sd_write()`
- `sd_chdir()` / `sd_getcwd()`

### Low-level Access (MMIO)
If you are writing assembly code, you no longer need to manipulate VIA pins. Simply use the MMIO registers:

```assembly
; SD Card Registers
SD_CTRL = $5008
SD_DATA = $5009

; Select the card
LDA #$01
STA SD_CTRL

; Transfer a byte
LDA #$40        ; CMD0
STA SD_DATA     ; Write starts transfer
LDA SD_DATA     ; Read returns MISO byte

; Deselect the card
LDA #$00
STA SD_CTRL
```
