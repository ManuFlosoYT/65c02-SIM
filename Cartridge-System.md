# Cartridge System v2.1

The **65c02-SIM** cartridge system allows packaging ROMs, assets, and hardware configurations into a single `.65c` file (which is a standard ZIP archive with a `manifest.json`).

## Packaging Tool: `create-cartridge.sh`

./scripts/create-cartridge.sh [ROM_FILE] [flags]

### Parameters

| Flag | Description | Default |
|------|-------------|---------|
| `--name` | Name of the cartridge | Filename of ROM |
| `--author` | Author name | "Unknown" |
| `--desc` | Brief description | "" |
| `--version` | Cartridge version | "1.0" |
| `--rom` | Path to the ROM `.bin` file | (Positional arg 1) |
| `--vram` | Path to a VRAM image binary (`vram.bin`) | None |
| `--ips` | Target Instructions Per Second (IPS) | 1,000,000 |
| `--gpu` | Enable GPU hardware (`true`/`false`) | `false` |
| `--cycle` | Enable Cycle-Accurate mode (`true`/`false`) | `true` |
| `--sid` | Enable SID sound hardware (`true`/`false`) | `true` |
| `--esp` | Enable ESP8266 WiFi module (`true`/`false`) | `false` |
| `--sd` | Enable SD Card emulation (`true`/`false`) | `false` |
| `--sd-image` | Path to an external SD image to bundle as `sdcard.img` | None |

### Cartridge Version 3.0 (Automatic SD)

Version 3.0 introduces support for integrated SD card images. When a cartridge contains an `sdcard.img` file:
1. The emulator extracts it automatically to a temporary path.
2. The SD card is mounted and managed by the cartridge.
3. **Persistence**: Any changes made to the SD card are saved back into the `.65c` (ZIP) archive when the cartridge is ejected or the application is closed.

### Usage with SD Image
```bash
./scripts/create-cartridge.sh output/rom/microDOS.bin --sd-image output/img/microDOS.img --sd true
```
```bash
./scripts/create-cartridge.sh --vram output/vram/logo.bin --name "LogoDemo" --gpu true
```

### Example: Full Hardware Configuration (microDOS)
```bash
./scripts/create-cartridge.sh output/rom/microDOS.bin --name "microDOS" --esp true --sd true --gpu true
```

---

## Technical Details

### Manifest Structure (`manifest.json`)
The manifest defines the cartridge version (v2.1), metadata, file mappings, and the hardware configuration.

```json
{
  "version": "2.1",
  "metadata": {
    "name": "MyGame",
    "author": "Dev",
    "description": "A 6502 Game",
    "version": "1.0"
  },
  "rom": "rom.bin",
  "vram": "vram.bin",
  "config": {
    "target_ips": 1000000,
    "gpu_enabled": true,
    "cycle_accurate": true,
    "sid_enabled": true,
    "esp_enabled": true,
    "sd_enabled": false
  },
  "bus": [
     { "name": "RAM", "start": "0x0000", "end": "0x7FFF" },
     { "name": "ROM", "start": "0x8000", "end": "0xFFFF" }
     ...
  ]
}
```

### Hardware Accurate VRAM Loading
The system supports direct VRAM loading. When a `vram.bin` is present in the cartridge:
1. The emulator loads it **after** the hardware reset to prevent data loss.
2. It uses a **128-byte stride** mapping to ensure visual parity with the GPU hardware's internal memory layout.
3. The UI (VRAM Viewer) is automatically refreshed to show the image immediately.

### Dynamic Device Activation
Hardware devices are registered on the bus dynamically based on the `config` section:
- **ESP8266**: Mapped to `0x5004-0x5007` if `esp_enabled` is true.
- **SD Card**: Activated as a virtual SPI device if `sd_enabled` is true.
- **Conflict Resolution**: If the **SD Card** is enabled, the virtual **LCD** is automatically deactivated to prevent hardware resource/pin conflicts.

### Building via SDK
The `scripts/create-sdk.sh` script automatically packages all binaries in the project using these flags, ensuring that system tools like `testNET` and `microDOS` have their respective hardware (WiFi/SD) enabled by default.

## microDOS Special Automation: `create-microdos-cartridge.sh`

For microDOS, a specialized script `create-microdos-cartridge.sh` is provided. This script:
1. Compiles the microDOS kernel and all applications in `sdk/microdosapps/Apps/`.
2. Creates a fresh 32MB FAT16 SD image.
3. Pre-populates the SD image with the compiled `.app` binaries in a `/bin` directory.
4. Packages everything into a legal v3.0 `microDOS.65c` cartridge.

**Dependencies**: Requires `dosfstools` and `mtools` installed on the host system.
