# Cartridge System Specification (`.65c`)

**Source Files:**  
- Data Model: [`src/Hardware/Core/Cartridge.h`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Hardware/Core/Cartridge.h)  
- Loader Service: [`src/Hardware/Core/CartridgeLoader.h`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Hardware/Core/CartridgeLoader.h) & [`CartridgeLoader.cpp`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Hardware/Core/CartridgeLoader.cpp)  
- Manifest Parser: [`src/Hardware/Core/CartridgeManifestParser.h`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Hardware/Core/CartridgeManifestParser.h) & [`CartridgeManifestParser.cpp`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Hardware/Core/CartridgeManifestParser.cpp)  
- Archive I/O: [`src/Hardware/Core/CartridgeZipIO.h`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Hardware/Core/CartridgeZipIO.h) & [`CartridgeZipIO.cpp`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Hardware/Core/CartridgeZipIO.cpp)  
- Packaging Tools: [`sdk/scripts/create-cartridge.sh`](file:///home/manu/Documents/GitHub/65c02-SIM/sdk/scripts/create-cartridge.sh) & [`sdk/scripts/create-microdos-cartridge.sh`](file:///home/manu/Documents/GitHub/65c02-SIM/sdk/scripts/create-microdos-cartridge.sh)

---

## 1. Overview

The **65c02-SIM** cartridge system packages software binaries, hardware memory mappings, initial graphics framebuffers, sound chips, peripheral states, FAT16 storage volumes, and debug symbol databases into a single, portable `.65c` cartridge file.

Under the hood, a `.65c` cartridge is a standard **ZIP archive** containing a `manifest.json` descriptor file along with binary assets. When loaded, the simulator configures the memory bus, flashes ROM, configures target clock speeds, mounts storage volumes, and links debugging symbols automatically.

```
MyGame.65c (ZIP Archive)
├── manifest.json       ← Cartridge hardware & bus configuration
├── rom.bin             ← 65C02 binary payload mapped into ROM
├── rom.dbg             ← CC65 debug symbols (optional)
├── rom.elf             ← ELF/DWARF debug symbols (optional)
├── vram.bin            ← Initial 100x75 VRAM graphics surface (optional)
└── sdcard.img          ← Bundled FAT16 virtual SD Card image (optional)
```

---

## 2. Manifest Schema (`manifest.json` v2.2)

The manifest file defines the cartridge metadata, file references, hardware execution parameters, and memory bus device assignments.

### 2.1 Complete JSON Specification

```json
{
  "version": "2.2",
  "metadata": {
    "name": "microDOS Operating System",
    "author": "ManuFloso",
    "description": "microDOS graphical operating system with bundled tools and games",
    "version": "7.0",
    "type": "rom"
  },
  "rom": "rom.bin",
  "dbg": "rom.dbg",
  "elf": "rom.elf",
  "vram": "vram.bin",
  "config": {
    "target_ips": 10000000,
    "gpu_enabled": true,
    "cycle_accurate": true,
    "sid_enabled": true,
    "sid_model": "8580",
    "esp_enabled": true,
    "sd_enabled": true
  },
  "bus": [
    { "name": "RAM", "start": "0x0000", "end": "0x7FFF" },
    { "name": "ROM", "start": "0x8000", "end": "0xFFFF" },
    { "name": "ACIA", "start": "0x5000", "end": "0x5003" },
    { "name": "VIA", "start": "0x6000", "end": "0x600F" },
    { "name": "ESP8266", "start": "0x5004", "end": "0x5007" },
    { "name": "SD Card", "start": "0x5008", "end": "0x500B" },
    { "name": "SID", "start": "0x4800", "end": "0x481F" },
    { "name": "GPU", "start": "0x2000", "end": "0x3FFF" }
  ]
}
```

### 2.2 Field Definitions

#### Root Fields
| Field | Type | Required | Description |
| :--- | :--- | :---: | :--- |
| `version` | String | Yes | Manifest specification version (`"2.1"` or `"2.2"`). |
| `metadata`| Object | Yes | Descriptive information (name, author, description, version, type). |
| `rom` | String | Optional* | File name of the CPU ROM binary inside the archive (e.g. `"rom.bin"`). |
| `vram` | String | Optional* | File name of initial GPU VRAM binary (e.g. `"vram.bin"`). |
| `dbg` / `elf` / `symbols` | String | Optional | File name of debug symbols (`.dbg` for CC65 or `.elf` for DWARF). If omitted, automatically checks for `rom.dbg` and `rom.elf`. |
| `config` | Object | Yes | Hardware configuration flags and clock parameters. |
| `bus` | Array | Yes | Ordered array of memory mapped bus device configurations. |

*\*Note: A cartridge must supply at least one of `rom` or `vram`.*

#### Hardware Configuration (`config`)
| Field | Type | Default | Description |
| :--- | :--- | :---: | :--- |
| `target_ips` | Integer | `1000000` | Target execution speed in Instructions Per Second (e.g. 1 MHz or 10 MHz). |
| `gpu_enabled` | Boolean | `false` | Enables graphics hardware and frame buffer scanning. |
| `cycle_accurate`| Boolean | `true` | Enables cycle-accurate sub-cycle synchronization across peripherals. |
| `sid_enabled` | Boolean | `true` | Enables MOS SID sound synthesis hardware. |
| `sid_model` | String/Int | `"8580"` | Selects SID chip generation: `"6581"` (MOS 6581) or `"8580"` (MOS 8580). Can also be overridden per-device in the bus array. |
| `esp_enabled` | Boolean | `false` | Enables ESP8266 WiFi serial modem peripheral. |
| `sd_enabled` | Boolean | `false` | Enables SPI virtual SD card mass storage. |

> [!TIP]
> Addresses in the `bus` array (`start` and `end`) can be specified either as hexadecimal strings (`"0x8000"`, `"0x5000"`) or as standard JSON integer literals (`32768`).

---

## 3. Bus Architecture & Overlap Validation

The memory bus configuration allows cartridges to customize the 64 KB address space for different system boards (Standard Architecture vs. microDOS Architecture).

### 3.1 Allowed Device Names & Capacity Limits
[`Core::CartridgeManifestParser`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Hardware/Core/CartridgeManifestParser.h) enforces strict device naming and hardware capacity limits:

| Device Name | Maximum Size | Typical Base Address | Hardware Description |
| :--- | :---: | :--- | :--- |
| `RAM` | 32 KB (`$8000`) | `$0000 - $7FFF` | System Static RAM (Zero Page, Stack, User RAM) |
| `ROM` | 32 KB (`$8000`) | `$8000 - $FFFF` | System Firmware ROM & Interrupt Vectors |
| `ACIA` | 4 Bytes (`$0004`) | `$5000` or `$7FC0` | W65C51N Serial UART Communications Controller |
| `VIA` | 16 Bytes (`$0010`)| `$6000` or `$7FD0` | W65C22 Versatile Interface Adapter (I/O, Timers) |
| `ESP8266` / `ESP32` | 4 Bytes (`$0004`) | `$5004` or `$7FC4` | WiFi Networking Coprocessor |
| `SD Card` | 4 Bytes (`$0004`) | `$5008` or `$7FC8` | SPI Mass Storage Host Controller |
| `SID` | 32 Bytes (`$0020`)| `$4800` or `$7FE0` | MOS 6581 / 8580 Sound Interface Device |
| `GPU` | 8 KB (`$2000`) | `$2000 - $3FFF` | 100x75 Standard Color Framebuffer |
| `microDOS-GPU`| 8 KB (`$2000`) | `$5FC0 - $7FBF` | microDOS Windowing Video Memory |
| `LCD` | Dynamic | VIA Port B | HD44780 2x16 Character Display |

### 3.2 Overlap Validation Algorithm
To prevent bus conflicts, [`CartridgeManifestParser::ValidateNoInvalidOverlaps()`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Hardware/Core/CartridgeManifestParser.h#L20) sorts device entries by start address and validates bounds:

1. **Mandatory Devices**: Every bus configuration must define both a `RAM` device and a `ROM` device.
2. **Device Segregation**: Devices are segregated into **Memory Blocks** (`RAM`, `ROM`) and **MMIO Peripherals** (`ACIA`, `VIA`, `SID`, `GPU`, `ESP8266`, `SD Card`).
3. **Collision Detection**:
   - Memory blocks cannot overlap other memory blocks ($start \le prev_{end}$).
   - MMIO peripherals cannot overlap other MMIO peripherals.
   - Any overlap within the same category triggers an immediate validation exception during cartridge mount.

---

## 4. Hardware-Accurate VRAM Loading

When a cartridge includes a `vram` image (e.g. splash logos or OS wallpapers):
1. **Deferred Initialization**: The VRAM binary payload is injected **after** the CPU hardware reset cycle completes, ensuring the zero-page initialization routine does not clear display memory.
2. **128-Byte Stride Layout**: VRAM bytes are mapped directly into the GPU frame buffer at a 128-byte stride layout matching the physical hardware's memory addressing.
3. **Immediate Display Refresh**: The OpenGL texture backing the VRAM viewer is immediately updated to display the image on the first frame.

---

## 5. Bundled FAT16 SD Image & Persistence

Cartridge versions 3.0+ support bundled virtual SD card storage volumes.

```
                    ┌────────────────────────────┐
                    │      GameCartridge.65c     │
                    │   (ZIP Archive on Disk)    │
                    └─────────────┬──────────────┘
                                  │
                       1. Load: Extract to Temp
                                  │
                                  ▼
                    ┌────────────────────────────┐
                    │  /tmp/65c02sim_game.img    │ ◄─── Read / Write Sectors
                    │    (Active 32MB FAT16)     │      via SPI Controller
                    └─────────────┬──────────────┘
                                  │
                   2. Eject/Exit: SaveSDToZip()
                      Atomic Rename (.tmp -> .bak)
                                  │
                                  ▼
                    ┌────────────────────────────┐
                    │      GameCartridge.65c     │
                    │  (Updated ZIP with Saves)  │
                    └────────────────────────────┘
```

### 5.1 Mount & Lifecycle
1. When `sdcard.img` is present in the ZIP archive, [`Core::CartridgeZipIO::ExtractSDCard()`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Hardware/Core/CartridgeZipIO.cpp#L189) extracts it to a temporary path (`/tmp/65c02sim_<name>.sdcard.img`).
2. The virtual [`SDCard`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Hardware/Storage/SDCard.h) hardware controller mounts the raw disk image for SPI read and write sector commands.

### 5.2 Safe Atomic Persistence ([`SaveSDToZip`](file:///home/manu/Documents/GitHub/65c02-SIM/src/Hardware/Core/CartridgeZipIO.cpp#L260))
When the user ejects the cartridge or exits the emulator:
1. The modified SD image is compressed and written into a temporary archive (`.tmp`).
2. The original `.65c` archive is moved to a backup (`.bak`).
3. The `.tmp` file is renamed to replace the `.65c` archive.
4. If an unexpected power failure or disk error occurs during packing, the backup is automatically restored, preventing cartridge corruption.

---

## 6. Packaging Tools & Automation

### 6.1 `create-cartridge.sh`
Located at [`sdk/scripts/create-cartridge.sh`](file:///home/manu/Documents/GitHub/65c02-SIM/sdk/scripts/create-cartridge.sh), this script bundles raw binaries into a `.65c` archive:

```bash
./sdk/scripts/create-cartridge.sh <ROM_FILE> [options]
```

#### Parameters

| Flag | Argument | Default | Description |
| :--- | :--- | :---: | :--- |
| `--name` | String | Filename | Human-readable title of the cartridge. |
| `--author` | String | `"Unknown"` | Author or developer name. |
| `--desc` | String | `""` | Brief description of the cartridge software. |
| `--version`| String | `"1.1"` | Semantic version tag. |
| `--rom` | Path | Positional #1 | Path to the 65C02 binary firmware file (`.bin`). |
| `--dbg` | Path | Auto-detect | Path to CC65 debug symbols (`.dbg`). |
| `--elf` | Path | Auto-detect | Path to DWARF / ELF symbols (`.elf`). |
| `--vram` | Path | None | Path to 100x75 VRAM binary payload (`.bin`). |
| `--sd-image`| Path | None | Path to 32MB FAT16 SD disk image (`.img`). |
| `--ips` | Integer | `1000000` | Target execution speed in instructions per second. |
| `--gpu` | Boolean | `false` | Enable GPU hardware rendering (`true`/`false`). |
| `--cycle` | Boolean | `true` | Enable cycle-accurate CPU timings. |
| `--sid` | Boolean | `true` | Enable MOS SID sound hardware. |
| `--sid-model`| String | `"8580"` | SID chip revision (`"6581"` or `"8580"`). |
| `--esp` | Boolean | `false` | Enable ESP8266 WiFi module. |
| `--sd` | Boolean | `false` | Enable SPI SD card peripheral. |
| `--microdos`| Flag | None | Configures microDOS bus memory addresses. |

#### Example: Packaging a Game Cartridge
```bash
./sdk/scripts/create-cartridge.sh build/invaders.bin \
    --name "Space Invaders" \
    --author "RetroDev" \
    --desc "Arcade port for 65c02-SIM" \
    --vram assets/title.bin \
    --ips 2000000 \
    --gpu true \
    --sid true \
    --sid-model "6581"
```

---

### 6.2 `create-microdos-cartridge.sh`
Located at [`sdk/scripts/create-microdos-cartridge.sh`](file:///home/manu/Documents/GitHub/65c02-SIM/sdk/scripts/create-microdos-cartridge.sh), this script automates full operating system compilation:

1. **Kernel Compilation**: Compiles the microDOS operating system kernel.
2. **Application Compilation**: Compiles all applications in `sdk/microdos/Apps/*.c` with `-Os` size optimization, generating icons from PNG/BMP sources.
3. **Desktop System**: Builds `DESKTOP.SYS` and helper libraries.
4. **FAT16 Image Creation**: Uses `mkfs.fat` to format a fresh 32MB disk image with 4 reserved sectors, 8 sectors per cluster, and 512 root entries.
5. **Asset Population**: Uses `mtools` (`mmd`, `mcopy`) to populate `/bin`, `/sid`, `/vram`, and `/SYSTEM` directories on the disk image.
6. **Cartridge Bundling**: Invokes `create-cartridge.sh` with `--microdos` bus parameters to output `output/cartridge/microDOS.65c`.
