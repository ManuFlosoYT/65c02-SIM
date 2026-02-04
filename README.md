<div align='center'>

<h1>65c02 emulator for the BenEater memory layout implementation</h1>
<p>65c02 emulator for the BenEater memory layout implementation</p>

<h4> <span> · </span> <a href="https://github.com/ManuFlosoYT/65c02-SIM/blob/master/README.md"> Documentation </a> <span> · </span> <a href="https://github.com/ManuFlosoYT/65c02-SIM/issues"> Report Bug </a> <span> · </span> <a href="https://github.com/ManuFlosoYT/65c02-SIM/issues"> Request Feature </a> </h4>


</div>

# :notebook_with_decorative_cover: Table of Contents

- [About the Project](#star2-about-the-project)
- [Contributing](#wave-contributing)
- [Contact](#handshake-contact)
- [Acknowledgements](#gem-acknowledgements)


## :toolbox: Getting Started

### :bangbang: Prerequisites

- CMake<a href="https://github.com/Kitware/CMake"> Here</a>
```bash
sudo dnf install cmake
```
- CC65<a href="https://github.com/cc65/cc65"> Here</a>
```bash
sudo dnf install cc65
```

### :running: Run Locally

Clone the project

```bash
git clone https://github.com/ManuFlosoYT/65c02-SIM
```
Build the project
```bash
./build.sh
```

Compile and run programs
```bash
./run.sh
```


## :wave: Contributing

Contributions are always welcome!

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

* **Microsoft BASIC (MSBASIC):** The assembly files located in `Linker/msbasic/` are derivative works of the original Microsoft BASIC for the 6502 (Copyright © 1977 Microsoft). These files are based on the restoration work by [mist64/msbasic](https://github.com/mist64/msbasic) and the adaptations by [Ben Eater](https://github.com/beneater/msbasic).
* **WozMon:** The file `Linker/wozmon.s` contains the source code for the Apple 1 System Monitor (WozMon), originally written by Steve Wozniak (Copyright © 1976 Apple Computer, Inc.).

These files are included for educational and preservation purposes to demonstrate the emulator's capabilities.