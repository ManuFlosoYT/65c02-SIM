<div align='center'>

<h1>65c02 emulator for the BenEater memory layout implementation</h1>
<p>Emulated components: CPU, VIA, ACIA, SID, GPU, 32KB RAM, 32KB ROM, 7,5KB VRAM (6,25KB usable by CPU)</p>

<h4> <span> · </span> <a href="https://github.com/ManuFlosoYT/65c02-SIM/wiki"> Documentation </a> <span> · </span> <a href="https://github.com/ManuFlosoYT/65c02-SIM/issues"> Report Bug </a> <span> · </span> <a href="https://github.com/ManuFlosoYT/65c02-SIM/issues"> Request Feature </a> </h4>

[![Build and Release](https://github.com/ManuFlosoYT/65c02-SIM/actions/workflows/release.yml/badge.svg)](https://github.com/ManuFlosoYT/65c02-SIM/actions/workflows/release.yml)

</div>

## :toolbox: Getting Started

### :running: Run Pre-compiled Binaries

Go to releases and download the latest release (and SDK if needed). [Here](https://github.com/ManuFlosoYT/65c02-SIM/releases)

#### Run the Simulator

Run the application and select the binary file you want to run.

### :package: SDK

Each release includes an `SDK.zip` file containing everything needed to develop for this simulator:

- `Binaries/`: Example C and Assembly programs.
- `Linker/`: Linker configurations and BIOS.
- `GPU/`: GPU assets and conversion tools.
- `SID/`: Sound assets and conversion tools.
- `compile-bin.sh`: Script to compile programs.
- `image-to-bin.sh`: Script to convert images.
- `midi-to-bin.sh`: Script to convert MIDI files.

#### Windows Users (WSL2)

Since all the provided automation scripts (`.sh`) are written for **Bash**, Windows users need to set up **WSL2 (Windows Subsystem for Linux)** to use the SDK or to compile the project from source.

To install Fedora on Windows 10/11:

1. Open **PowerShell** or **Command Prompt** as Administrator.
2. Run the following command:
   ```powershell
    wsl --install FedoraLinux-43
   ```
3. Open the newly installed **Fedora** application and configure your user account.
4. Install the requiered dependencies from the prerequisites section bellow.

Once set up, you can run all the commands described below (and in the [Build from Source](#hammer_and_wrench-build-from-source) section) directly from the Fedora terminal.

Compile and run assembly or C programs from `./Binaries/` into the `output/rom` folder:

```bash
./compile-bin.sh <program_name>
```

_(Note: This tool automatically handles both `.s` and `.c` files, and also the special `eater` ROM target.)_

Convert images to VRAM binary format into the `output/vram` folder:

```bash
./image-to-bin.sh <image_name>
```

Convert MIDI files to SID assembly code into the `output/midi` folder:

```bash
./midi-to-bin.sh <midi_file>
```

### :hammer_and_wrench: Build from Source

#### Prerequisites

To compile the simulator from source or use the SDK, you need the following dependencies:

**Fedora (`dnf`)**

```bash
sudo dnf update
sudo dnf install gcc-c++ ninja-build mesa-libGL-devel mesa-libGLU-devel libX11-devel libXrandr-devel libXinerama-devel libXcursor-devel libXi-devel wayland-devel libxkbcommon-devel mingw64-gcc mingw32-gcc alsa-lib-devel pulseaudio-libs-devel cmake cc65 openssl-devel mingw64-openssl mingw64-openssl-static mingw64-zlib-static wget curl git tar gcc git-core make fribidi-devel pipewire-devel libXext-devel libXfixes-devel libXScrnSaver-devel libXtst-devel dbus-devel ibus-devel systemd-devel mesa-libGLES-devel mesa-libEGL-devel vulkan-devel wayland-protocols-devel libdrm-devel mesa-libgbm-devel libusb1-devel libdecor-devel pipewire-jack-audio-connection-kit-devel libthai-devel liburing-devel zlib-ng-compat-static python3-jinja2 ccache
```

#### Compile and Run Locally

Clone the project

```bash
git clone https://github.com/ManuFlosoYT/65c02-SIM
```

Build for Linux:

Build for Linux:

```bash
./build-linux.sh
# Check build errors or force a clean build
./build-linux.sh --clean
```

Build for Windows (using MinGW):

```bash
./build-win.sh
# Check build errors or force a clean build
./build-win.sh --clean
```

## :wave: Contributing

Contributions are always welcome!

The entire UnitTests/ folder has been AI generated, sorry for the mess. 😅😅

## :handshake: Contact

@ManuFloso (discord)

Project Link: [https://github.com/ManuFlosoYT/65c02-SIM](https://github.com/ManuFlosoYT/65c02-SIM)

## :gem: Acknowledgements

Use this section to mention useful resources and libraries that you have used in your projects.

- [Ben Eater](https://github.com/beneater/msbasic)
- [Obelisk 6502](http://www.6502.org/users/obelisk/6502/reference.html)
- [Masswerk 6502](https://www.masswerk.at/6502/6502_instruction_set.html)

## :balance_scale: License & Copyright

The emulator source code (C++) in this repository is licensed under the **MIT License**.

However, this project includes historical software and derivative works that are subject to their own copyrights:

- **Microsoft BASIC (MSBASIC):** The assembly files located in `Linker/msbasic/` are derivative works of the original Microsoft BASIC for the 6502 (Copyright © 1977 Microsoft). These files are based on the restoration work by [mist64/msbasic](https://github.com/mist64/msbasic) and the adaptations by [Ben Eater](https://github.com/beneater/msbasic).
- **WozMon:** The file `wozmon.s` contains the source code for the Apple 1 System Monitor (WozMon), originally written by Steve Wozniak (Copyright © 1976 Apple Computer, Inc.).

These files are included for educational and preservation purposes to demonstrate the emulator's capabilities.
