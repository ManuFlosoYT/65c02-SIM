# Cartridge System v2.1

The **65c02-SIM** cartridge system allows packaging ROMs, assets, and hardware configurations into a single `.65c` file (which is a standard ZIP archive with a `manifest.json`).

## Packaging Tool: `create-cartridge.sh`

The `create-cartridge.sh` script is the primary tool for creating cartridges. It automates the creation of the `manifest.json`, inclusion of binaries, and zipping.

### Usage

```bash
./create-cartridge.sh [ROM_FILE] [flags]
```

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

### Example: VRAM-only Cartridge
```bash
./create-cartridge.sh --vram output/vram/logo.bin --name "LogoDemo" --gpu true
```

### Example: Full Hardware Configuration (microDOS)
```bash
./create-cartridge.sh output/rom/microDOS.bin --name "microDOS" --esp true --sd true --gpu true
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
The `create-sdk.sh` script automatically packages all binaries in the project using these flags, ensuring that system tools like `testNET` and `microDOS` have their respective hardware (WiFi/SD) enabled by default.
