# SD Card Emulation

The emulator includes a high-level emulation of an SD Card interface through SPI (Serial Peripheral Interface) bit-banging. This allows programs to store and retrieve data from a virtual SD card image file in FAT16 format.

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

## Hardware Interface (VIA Wiring)

The SD Card hardware is not memory-mapped directly. Instead, it is connected to the **VIA (6522)** chip on **Port B**. The 65C02 communicates with the card by bit-banging the SPI protocol through the following pins:

| VIA Pin | Function | Description |
| ------- | -------- | ----------- |
| **PB0** | **MOSI** | Master Out Slave In |
| **PB1** | **MISO** | Master In Slave Out |
| **PB2** | **SCK**  | Serial Clock |
| **PB3** | **CS**   | Chip Select (Active Low) |

> [!NOTE]
> The emulator automatically handles the bit-sync and byte-exchange when it detects activity on these pins, provided the SD Card hardware is enabled.

## Enabling the Hardware

By default, the SD Card hardware is **disabled** to maintain a simpler environment for basic 6502 learning. To use the SD Card in your programs, follow these steps:

1.  Open the **Memory Layout** window from the emulator's GUI.
2.  Scroll down to the **Virtual Devices** section.
3.  Find the **SD Card** entry and check the **Enabled** box. (It will collide with the **LCD** as both use **PORTB**)
4.  Click the **Apply & Reset Emulator** button at the bottom of the window.
5.  Click the **Settings** button and create/choose a `.img` file for the SD Card.

The emulator will now allow SPI communication through VIA Port B.

> [!NOTE]
> Try running the `testSD` program from the SDK to test the SD Card functionality.

## Software Support

### SDK Library
The project's SDK includes a high-level wrapper for the FatFs library. You can use it by including `Libs/SD.h` in your C programs. It provides standard functions like:
- `sd_mount()`
- `sd_ls()`
- `sd_open()` / `sd_read()` / `sd_write()`
- `sd_chdir()` / `sd_getcwd()`

### Low-level Access
If you are writing assembly code, you must manually manipulate the VIA Port B registers (`0x6000`, `0x6002`) to clock bits in and out.
