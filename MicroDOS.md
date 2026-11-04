# microDOS Operating System Manual

microDOS is a full-featured, lightweight operating system for the 65C02 microprocessor within the 65c02-SIM platform. Written in modern C (compiled with LLVM-MOS) and assembly, microDOS provides a monolithic kernel, a hybrid graphical desktop environment (`DESKTOP.SYS`), an interactive Unix-style TUI shell with pipelining, full FAT16 filesystem support via FatFs, virtual memory paging, audio synthesis, and networking.

---

## 1. System Architecture

microDOS combines an embedded monolithic kernel with modular user-space execution. The kernel resides in high ROM ($8000–$FFFF) and maintains a fixed 35-function **ABI Jump Table** at `$FF90`, allowing user applications (`.app` binaries) to invoke kernel services without recompilation across kernel revisions.

```
+-------------------------------------------------------------------------+
|                              User Layer                                 |
|   +-----------------------+     +-----------------------------------+   |
|   | DESKTOP.SYS (GUI/App) |     |  CLI Shell & 40+ Built-in Apps    |   |
|   | 12-App Grid, Mouse    |     |  nano, word, basic, play, ftp...  |   |
|   +-----------------------+     +-----------------------------------+   |
+-------------------------------------------------------------------------+
|                           microDOS Kernel (ROM)                         |
|   +-----------------------------------------------------------------+   |
|   |                     ABI Jump Table ($FF90)                      |   |
|   +-------------------+--------------------+------------------------+   |
|   |   FatFs (FAT16)   |    Virtual Memory  |    Serial / TUI / VRAM |   |
|   |   File Operations |    Overlays & Swap |    I/O & Print Drivers |   |
+---+-------------------+--------------------+------------------------+---+
|                              Hardware Layer                             |
|       W65C02S CPU  |  microDOS-GPU  |  6522 VIA  |  MOS 6581/8580 SID   |
|       ACIA 6850    |  ESP8266 WiFi  |  SD Card (SPI Mode)               |
+-------------------------------------------------------------------------+
```

### Key Architectural Subsystems
- **Monolithic Kernel Core:** Manages startup initialization, SD card mounting, application binary loading, process lifecycle, overlay page swapping, and vector dispatch.
- **Graphical Desktop Environment (`DESKTOP.SYS`):** A loadable system shell executed from `/SYSTEM/DESKTOP/DESKTOP.SYS` at boot. It renders an 8KB wallpaper (`bliss.bin`), draws a 12-app paginated icon grid, handles mouse movement and button clicks, and launches selected `.app` executables.
- **TUI & Text Shell:** Fallback interactive shell supporting command history, argument parsing, pipe redirection (`|`), batch script processing (`.bat`), and full ANSI VT100 terminal emulation.
- **FatFs Storage Stack:** True FAT16 filesystem driver communicating over MMIO SPI registers with an SD card image, handling hierarchical directories, file handles, seeking, and streaming.
- **Virtual Memory Pager:** Transparent 8KB overlay paging window ($2800–$47FF) and dynamic file swapping in `/SYSTEM/SWAP/` for applications requiring more memory than available RAM.

---

## 2. Memory Map & Layout

microDOS uses a specialized memory layout designed to maximize application memory while providing hardware memory-mapped I/O and graphics.

```
0x0000 +---------------------------------------------+
       | Zero Page - App Variables ($0000-$005F)     |
0x0060 +---------------------------------------------+
       | App Argument Pointers ($0060-$0062)         |
0x0063 +---------------------------------------------+
       | Reserved / Scratch ($0063-$0077)            |
0x0078 +---------------------------------------------+
       | Zero Page - Kernel Variables ($0078-$00FB)  |
0x00FC +---------------------------------------------+
       | Circular Buffer Pointers ($00FC-$00FD)      |
0x0100 +---------------------------------------------+
       | CPU Hardware Call Stack ($0100-$01FF)       |
0x0200 +---------------------------------------------+
       | Safe App RAM ($0200-$53FF)                  |
       | - App Header & Code (.text) starts at $0200 |
       | - App Data & BSS (.data, .bss)              |
       | - App Software Stack ($53FF downwards)      |
0x2800 |   [Paged Overlay Window: $2800-$47FF (8KB)] |
0x5400 +---------------------------------------------+
       | microDOS Kernel RAM ($5400-$5FBF, ~3 KB)    |
       | - Kernel Work Buffers & FatFs Mount Data    |
       | - Kernel Software Stack ($5FBF downwards)   |
0x5FC0 +---------------------------------------------+
       | Extended RAM / microDOS-GPU VRAM (8 KB)     |
       | - 100x75 VRAM @ 1 byte/pixel ($5FC0-$7D0B)  |
       | - GPU Extra RAM ($7D0C-$7FAF)               |
       | - Host Mouse Registers ($7FB0-$7FB2)        |
       |   (MicroDosGPU internal offsets $1FF0-$1FF2)|
0x7FC0 +---------------------------------------------+
       | Memory-Mapped I/O (MMIO) Registers          |
       | - $7FC0-$7FC3: ACIA 6850 (Serial Console)   |
       | - $7FC4-$7FC7: ESP8266 WiFi Controller      |
       | - $7FC8-$7FCB: SD Card SPI Controller       |
       | - $7FD0-$7FDF: VIA 6522 Complex Interface   |
       | - $7FE0-$7FFF: MOS 6581 / 8580 SID Audio    |
0x8000 +---------------------------------------------+
       | microDOS Kernel ROM ($8000-$FFFF)           |
       | - Operating System Code & Drivers           |
       | - FatFs Implementation                      |
       | - ABI Jump Table ($FF90-$FFF9)              |
       | - Hardware Vectors ($FFFA-$FFFF)            |
0xFFFF +---------------------------------------------+
```

### Memory Segments Detail

| Address Range | Size | Allocation / Description |
|---|---|---|
| `$0000–$005F` | 96 B | Application Zero Page (LLVM-MOS registers `__rc0`–`__rc31` and app globals) |
| `$0060–$0062` | 3 B | Process Arguments: `$60` = `arg_count`, `$61–$62` = `_args_ptr` |
| `$0078–$00FB` | 132 B | microDOS Kernel Zero Page variables |
| `$00FC–$00FD` | 2 B | BIOS UART circular input buffer pointers (`READ_PTR`, `WRITE_PTR`) |
| `$0100–$01FF` | 256 B | 65C02 Hardware Stack |
| `$0200–$53FF` | 20.5 KB | **Safe App RAM:** Binary entry point at `$0200`. Apps without overlays can use this entire continuous span. Default app stack starts at `$53FF`. |
| `$2800–$47FF` | 8 KB | **Paged Overlay Window:** When an app defines overlay banks, this window dynamically loads 8KB pages from the binary. |
| `$5400–$5FBF` | 3008 B | **Kernel RAM:** Operating system buffers, active file handles, and kernel stack (`__stack = 0x5FBF`). |
| `$5FC0–$7FBF` | 8 KB | **microDOS-GPU VRAM / Extended RAM:** When GPU is enabled, mapped to VRAM (100x75 pixels). When GPU is disabled, serves as 8KB extra heap for applications (`APP_EXTRA_RAM`). |
| `$7FB0–$7FB2` | 3 B | **Host Mouse Registers:** Hardware mouse registers (Buttons, X, Y). Within `MicroDosGPU`, these reside at internal offsets `$1FF0` (Buttons), `$1FF1` (X), `$1FF2` (Y). When mapped at base `$5FC0`, they appear at `$7FB0–$7FB2`. |
| `$7FC0–$7FFF` | 64 B | System Peripheral MMIO Space |
| `$8000–$FFFF` | 32 KB | microDOS Monolithic Kernel ROM |

---

## 3. microDOS ABI Jump Table Reference

Applications call operating system functions through an indirect Jump Table located at `$FF90` in Kernel ROM. When using the LLVM-MOS toolchain, these symbols are linked automatically via `tools/linker/generate_app_link_ld.py`.

The header file [`sdk/microdos/Libs/app-bios.h`](file:///home/manu/Documents/GitHub/65c02-SIM/sdk/microdos/Libs/app-bios.h) exposes C prototypes for all 35 jump table entries:

| Address | Function Symbol | Prototype / Signature | Description |
|---|---|---|---|
| `$FF90` | `print_str` | `void print_str(const char* s)` | Output null-terminated string to console |
| `$FF93` | `println` | `void println(const char* s)` | Print string followed by CRLF (`\r\n`) |
| `$FF96` | `print_num` | `void print_num(unsigned int n)` | Print unsigned 16-bit integer in decimal |
| `$FF99` | `print_hex_byte` | `void print_hex_byte(unsigned char v)` | Print 8-bit value as 2 hex digits |
| `$FF9C` | `bios_getchar` | `char bios_getchar(void)` | Read character from ACIA buffer (blocking) |
| `$FF9F` | `bios_putchar` | `void bios_putchar(char c)` | Send single character to ACIA / active pipe |
| `$FFA2` | `sd_open` | `uint8_t sd_open(SD_FILE* fp, const char* path, uint8_t mode)` | Open or create file (returns 1 on success) |
| `$FFA5` | `sd_read` | `int16_t sd_read(SD_FILE* fp, void* buf, uint16_t len)` | Read bytes from file (returns count read) |
| `$FFA8` | `sd_write` | `int16_t sd_write(SD_FILE* fp, const void* buf, uint16_t len)` | Write bytes to file (returns count written) |
| `$FFAB` | `sd_close` | `uint8_t sd_close(SD_FILE* fp)` | Close open file handle |
| `$FFAE` | `sd_puts` | `uint8_t sd_puts(SD_FILE* fp, const char* s)` | Write null-terminated string to file |
| `$FFB1` | `sd_getc` | `int16_t sd_getc(SD_FILE* fp)` | Read single char (-1 on EOF) |
| `$FFB4` | `sd_getcwd` | `uint8_t sd_getcwd(char* buf, uint16_t len)` | Retrieve current working directory |
| `$FFB7` | `sd_chdir` | `uint8_t sd_chdir(const char* path)` | Change working directory |
| `$FFBA` | `net_send` | `void net_send(const char* s)` | Send string to ESP8266 UART |
| `$FFBD` | `net_cmd` | `void net_cmd(const char* s)` | Send AT command to ESP8266 |
| `$FFC0` | `net_send_num` | `void net_send_num(uint16_t n)` | Send integer string to ESP8266 |
| `$FFC3` | `os_load_app_page`| `void os_load_app_page(uint8_t page_id)` | Load overlay page into `$2800` window |
| `$FFC6` | `sid_write` | `void sid_write(uint8_t reg, uint8_t val)` | Write byte to SID sound register |
| `$FFC9` | `sid_reset` | `void sid_reset(void)` | Reset and silence all 3 SID voices |
| `$FFCC` | `sd_opendir` | `int sd_opendir(SD_DIR* dp, const char* path)` | Open directory for iteration |
| `$FFCF` | `sd_readdir` | `int sd_readdir(SD_DIR* dp, SD_INFO* fno)` | Read next directory entry |
| `$FFD2` | `sd_closedir` | `void sd_closedir(SD_DIR* dp)` | Close directory handle |
| `$FFD5` | `sd_mkdir` | `uint8_t sd_mkdir(const char* path)` | Create new directory |
| `$FFD8` | `sd_remove` | `uint8_t sd_remove(const char* path)` | Delete file or empty directory |
| `$FFDB` | `sd_exists` | `uint8_t sd_exists(const char* path)` | Check if path exists on SD card |
| `$FFDE` | `net_wifi` | `void net_wifi(const char* ssid, const char* pass)` | Join WiFi access point via ESP8266 |
| `$FFE1` | `os_swap_save` | `uint8_t os_swap_save(const char* name, void* addr, uint16_t size)` | Save memory block to `/SYSTEM/SWAP/<name>.SWP` |
| `$FFE4` | `os_swap_load` | `uint8_t os_swap_load(const char* name, void* addr, uint16_t size)` | Restore memory block from swap |
| `$FFE7` | `os_alloc` | `void* os_alloc(uint16_t size)` | Allocate block from dynamic heap |
| `$FFEA` | `os_free` | `void os_free(void* ptr)` | Free heap memory block |
| `$FFED` | `bios_getchar_nb`| `char bios_getchar_nb(void)` | Non-blocking read (returns 0 if empty) |
| `$FFF0` | `sd_seek` | `int sd_seek(SD_FILE* fp, uint32_t pos)` | Seek file read/write pointer |
| `$FFF3` | `os_redirect_out`| `void os_redirect_out(const char* path, uint8_t append)`| Redirect stdout to file (`>` or `>>`) |
| `$FFF6` | `os_redirect_in` | `void os_redirect_in(const char* path)` | Redirect stdin from file (`<`) |

---

## 4. Hardware Interfaces & Registers in microDOS

microDOS re-maps peripheral MMIO into the range `$7FC0–$7FFF` to free lower memory for applications:

### 1. ACIA 6850 Console (`$7FC0–$7FC3`)
- `$7FC0`: ACIA Data Register (Read RX / Write TX)
- `$7FC1`: ACIA Status Register (Bit 0 = RX Full, Bit 1 = TX Empty, Bit 7 = IRQ)
- `$7FC2`: Command Register
- `$7FC3`: Control Register

### 2. ESP8266 WiFi (`$7FC4–$7FC7`)
- `$7FC4`: `ESP8266_DATA` — Read received byte / Write byte to transmit
- `$7FC5`: `ESP8266_STATUS` — Bit 7 (`0x80`): Data available flag (`net_has_data()`)
- Convenience macros in `app-bios.h`:
  ```c
  #define net_has_data() (ESP8266_STATUS & 0x80)
  #define net_putc(c)    (ESP8266_DATA = (c))
  #define net_getc()     (ESP8266_DATA)
  ```

### 3. SD Card SPI Interface (`$7FC8–$7FCB`)
- `$7FC8`: SPI Data Transfer (Write initiates transmission, Read returns received byte)
- `$7FC9`: SPI Status / Control (Chip Select control)

### 4. VIA 6522 (`$7FD0–$7FDF`)
- `$7FD0`: `PORTB` / `DDRB` — Peripheral port B (used by LCD data/control lines)
- `$7FD1`: `PORTA` / `DDRA` — Hardware handshake & status lines
- `$7FD4–$7FD5`: Timer 1 Counter (Low/High)
- `$7FD8–$7FD9`: Timer 2 Counter (Low/High)

### 5. MOS 6581/8580 SID Audio (`$7FE0–$7FFF`)
29 audio synthesis registers controlling 3 independent voices:
- Voice 1: `$7FE0–$7FE6` (Freq Low/High, Pulse Width Low/High, Control, Attack/Decay, Sustain/Release)
- Voice 2: `$7FE7–$7FED`
- Voice 3: `$7FEE–$7FF4`
- Filter Cutoff: `$7FF5–$7FF6` (`FC_LO`, `FC_HI`)
- Filter Resonance & Routing: `$7FF7` (`RES_FILT`)
- Mode & Master Volume: `$7FF8` (`MODE_VOL`)

Helper macros provided by `app-bios.h`:
```c
sid_trigger_note(voice, freq, ctrl); // Play frequency with waveform
sid_stop_note(voice);                // Clear gate bit
sid_set_envelope(voice, a, d, s, r); // Configure ADSR
sid_set_volume(vol);                 // Set master volume (0-15)
sid_set_filter_cutoff(cutoff);       // Configure analog filter cutoff
```

---

## 5. Graphical Desktop Environment (`DESKTOP.SYS`)

`DESKTOP.SYS` provides an intuitive graphical interface loaded directly into `$0200` by the kernel on boot.

```
+-------------------------------------------------------------------------+
| [icon 1]       [icon 2]       [icon 3]       [icon 4]     (Row 0: Y=3)  |
| Calc           Nano           Basic          Tetris                     |
|                                                                         |
| [icon 5]       [icon 6]       [icon 7]       [icon 8]     (Row 1: Y=21) |
| Word           Play           Dialog         View                       |
|                                                                         |
| [icon 9]       [icon 10]      [icon 11]      [icon 12]    (Row 2: Y=39) |
| Room           Game           Sysinfo        WiFi                       |
+-------------------------------------------------------------------------+
|  < [Page 1/4] >                    Active: TETRIS.APP      (Bar: Y=55)  |
+-------------------------------------------------------------------------+
```

### Technical Specifications
1. **Wallpaper Rendering:** Loads 8192 bytes from `/SYSTEM/DESKTOP/bliss.bin` into `GPU_VRAM_START` (`$5FC0`). If absent, clears the screen to black (`0x00`).
2. **App Grid Pagination:** Displays up to 12 applications per page arranged in a 4-column by 3-row grid:
   - Column X coordinates: `COL_X = {6, 30, 54, 78}`
   - Row Y coordinates: `ROW_Y = {3, 21, 39}`
   - Supports up to 255 applications scanned dynamically from `/bin/*.app`.
3. **16x16 Application Icons:** Reads the 16-byte `.app` header. If flag bit 1 (`APP_FLAG_HAS_ICON = 0x02`) is set, reads 256 bytes from offset 16 in the file directly into the VRAM rectangular window (`.header.1_icon`).
4. **Desktop Exclusion Flag:** Applications compiled with `MICRODOS_NO_DESKTOP()` (e.g. headless utilities like `cat`, `rm`, `nano`) set flag bit 2 (`APP_FLAG_NO_DESKTOP = 0x04`). `DESKTOP.SYS` filters these out during directory scan.
5. **5x5 & 3x5 Bitmap Font Rendering Engine:**
   - Powered by [`sdk/microdos/Libs/Font5x5.h`](file:///home/manu/Documents/GitHub/65c02-SIM/sdk/microdos/Libs/Font5x5.h) and [`sdk/microdos/Libs/Font3x5.h`](file:///home/manu/Documents/GitHub/65c02-SIM/sdk/microdos/Libs/Font3x5.h).
   - **Glyph Bitmaps:** Each character in `Font5x5.h` consists of a 5x5 pixel matrix encoded across 5 bytes (`FONT_5X5_GLYPHS[95][5]`), covering 95 standard printable ASCII characters (ASCII 32 `' '` through 126 `'~'`).
   - **Spacing & Stride:** Fixed 6-pixel horizontal stride (`FONT_STRIDE_X = 6`) and 6-pixel vertical stride (`FONT_STRIDE_Y = 6`) to maintain clean text separation.
   - **Rendering API:**
     - `void font_draw_char(int8_t x, int8_t y, char c, uint8_t color)`: Plots individual glyph pixels using `gpu_put_pixel`.
     - `void font_draw_string(int8_t x, int8_t y, const char* str, uint8_t color)`: Renders full text strings with newline wrap (`\n`) handling.
     - `font_draw(x, y, item, [color])`: C11/C23 `_Generic` polymorphic macro that dispatches between characters and strings automatically, defaulting to white (`FONT_DEFAULT_COLOR = 0x3F`).
   - In `desktop_sys.c`, the font engine draws the application labels beneath each 16x16 icon, renders the page navigation indicators (`< [Page X/Y] >`), and displays the active application filename in the status bar.
6. **PS/2 Mouse Cursor Management:**
   - **Hardware Registers:** Coordinates and buttons are read in real-time from `MicroDosGPU`. Within the GPU controller, the registers reside at internal offsets:
     - `MOUSE_BTNS_OFFSET = 0x1FF0U`
     - `MOUSE_X_OFFSET    = 0x1FF1U`
     - `MOUSE_Y_OFFSET    = 0x1FF2U`
   - **65C02 MMIO Addresses:** When `microDOS-GPU` is mapped at base address `$5FC0`, these registers appear at CPU addresses `$7FB0` (`MOUSE_BTNS`), `$7FB1` (`MOUSE_X`), and `$7FB2` (`MOUSE_Y`).
   - **Cursor Logic:** Non-destructive 8x8 software cursor. Before drawing the pointer arrow, the desktop saves the 8x8 background pixels beneath the cursor into `cursor_bg[8][8]`. When the mouse moves or an app launches, it restores the background perfectly, preventing visual trails.
   - **Interaction:** Left click over an app icon executes the binary; clicking `<` or `>` advances the page.

---

## 6. Shell & Terminal User Interface (TUI)

If `DESKTOP.SYS` is absent or exited, microDOS drops into its CLI shell.

### Command Execution & Architecture
- **Interactive Prompt:** Displays `uDOS:<CWD>> ` showing current working directory.
- **Direct Execution:** Typing an application name (e.g. `ls`, `calc 2+2`, `nano doc.txt`) searches `/bin/<name>.app` and loads it into safe RAM starting at `$0200`.
- **Argument Passing:** Arguments are parsed by `tokenize(char* buf)` into `args[MAX_ARGS]` and passed to user apps via zero-page pointer `$61–$62` (`_args_ptr`) with argument count at `$60` (`arg_count`).

### Multi-Stage Pipelines (`|`)
microDOS implements a robust multi-stage pipeline mechanism (`desktop_dispatch_pipeline`) designed specifically for single-tasking 65C02 systems:
- When a command contains pipes (e.g. `cat file.txt | grep error | wc`), the pipeline dispatcher splits the command into individual stages.
- The first process is executed with standard output redirected (`os_redirect_out`) to a temporary file on the SD card (`_pipe1.tmp`).
- Subsequent stages are queued into a command file at `/_pipe.cmd`.
- When the first process finishes and returns control to the shell, `desktop_check_pending_pipe` detects the pending queue, sets standard input (`os_redirect_in`) from `_pipe1.tmp`, redirects output to `_pipe2.tmp`, and launches the next stage.
- Upon completion of the pipeline chain, `desktop_check_pipe_cleanup` removes all temporary files (`/_pipe.cln`, `_pipe*.tmp`).

### Tab Auto-Completion (`desktop_autocomplete`)
Pressing `Tab` (ASCII 9) during command entry triggers intelligent path and command completion:
- Parses the trailing token preceding the cursor.
- Resolves relative directory paths (`.`) or absolute paths (`/path/prefix`).
- Scans the target SD card directory using `sd_opendir` and `sd_readdir`.
- Performs case-insensitive prefix matching (`prefix_match_nocase`).
- When a match is found, auto-completes the remainder of the name directly on the screen and into the command buffer.
- Automatically appends a trailing slash (`/`) for directories or a space (`' '`) for files.

### Interactive Command History
- The shell tracks recently executed commands in a circular history buffer (`hist_entries[HIST_MAX][HIST_LEN]`).
- Decodes multi-byte ANSI terminal escape sequences:
  - **Up Arrow (`\x1b[A`):** Recalls previous commands, moving backward through history.
  - **Down Arrow (`\x1b[B`):** Moves forward toward newer commands, restoring the unsaved current draft line (`hist_draft`) when returning to the bottom.
- Emits terminal backspace sequences (`\b \b`) to cleanly erase and redraw the edited line in real time.
- Command history persists across shell sessions via `desktop_hist_load()`.

### Batch Script Processing (`.bat` / `.cmd`)
microDOS supports automated script execution:
- Executed by typing `batch <script.bat>` or directly executing any file ending in `.bat` or `.cmd`.
- Lines are queued into `/_batch.cmd` and processed sequentially. Each line executes as a fresh microDOS command with full pipeline and redirection support.
- **Startup Auto-Run:** On system boot, `desktop_scan_startup` checks for scripts in `/SYSTEM/STARTUP` or root `autoexec.bat` to configure environment settings, run self-tests, or preload drivers.

### TUI Library (`Libs/tui.h`)
The TUI library enables sophisticated text-based user interfaces:
- **ANSI Color Modes:** Standard 8 colors, 256-color palette (`tui_set_color_256`), and 24-bit TrueColor (`tui_set_color_rgb`).
- **Windowing Primitives:** `tui_draw_window`, `tui_draw_box`, `tui_draw_shadow`, `tui_draw_button`.
- **Input Handling:** `tui_get_key` parses multi-byte ANSI escape sequences for Arrow keys, Home, End, Delete, Tab, and Shift-Tab.

---

## 7. Complete Application Catalog (47 Built-in Apps)

All applications are located in `sdk/microdos/Apps/` and compiled to `/bin/*.app`:

| Application | Category | Description |
|---|---|---|
| `ascii.c` | Utilities | Visual standard ASCII table viewer (character codes 32–126). |
| `ascii2.c` | Utilities | Extended ASCII & IBM CP437 character set reference. |
| `basic.c` | Languages | Full Microsoft-compatible BASIC interpreter using 4 overlay pages. Supports floating point, strings, multi-dimensional arrays, and file I/O. |
| `boxes.c` | Graphics Demo | Graphical demo drawing animated color-cycling concentric rectangles. |
| `calc.c` | Utilities | Command-line expression evaluator and arithmetic calculator. |
| `cat.c` | File System | Concatenates and prints file contents to stdout. |
| `cd.c` | File System | Changes current working directory (`cd /bin`, `cd ..`). |
| `color.c` | Graphics Demo | Tests 64-color RGB222 palette and ANSI VT100 text colors. |
| `cp.c` | File System | Copies files from source to destination. |
| `df.c` | System | Reports SD card total size, used clusters, and free space. |
| `dialog.c` | TUI Demo | Interactive dialog box demonstration with modal buttons and keyboard focus. |
| `echo.c` | Utilities | Prints text arguments to console (supports stdout file redirection). |
| `ftp.c` | Networking | Interactive FTP client using ESP8266 WiFi for uploading and downloading files. |
| `game.c` | Games | 2D platformer jump-and-run game with physics, platform collision, and animation. |
| `grep.c` | Utilities | Regular text pattern search across files or standard input streams. |
| `hexdump.c` | Utilities | Displays binary files in formatted hexadecimal and ASCII columns. |
| `ls.c` | File System | Lists directory contents with file size and attributes. |
| `membench.c`| Benchmarks | Measures CPU memory read/write throughput (MB/s). |
| `memtest.c` | Diagnostics | Comprehensive RAM integrity tester running pattern tests over safe RAM. |
| `mkdir.c` | File System | Creates new directory paths on the SD card. |
| `more.c` | Utilities | Paged text viewer with keyboard navigation (`Space`, `Enter`, `q`). |
| `mv.c` | File System | Moves or renames files across directories. |
| `nano.c` | Editors | Full-screen text editor with cursor editing, backspace, and disk save (`ESC`). |
| `nc.c` | Networking | Netcat network utility for establishing raw TCP/IP client/server sockets via ESP8266. |
| `peek.c` | Debugging | Inspects and displays the byte value at any 16-bit physical memory address. |
| `ping.c` | Networking | Network ICMP/TCP ping latency measurement utility. |
| `play.c` | Multimedia | SID audio player for native `.sid` music files with real-time streaming decompression. |
| `poke.c` | Debugging | Writes a specified byte value directly to any 16-bit memory address. |
| `pwd.c` | File System | Prints the current working directory path. |
| `rm.c` | File System | Deletes files from the SD card. |
| `rmdir.c` | File System | Removes empty directories. |
| `room.c` | 3D Graphics | 3D wireframe room rendering demo using fixed-point matrix transforms. |
| `sdbench.c` | Benchmarks | Measures sequential read and write speeds on the SD card SPI interface. |
| `sidreg.c` | Multimedia | Direct diagnostic tool for reading and tweaking individual SID registers. |
| `spi.c` | Diagnostics | Low-level test of SPI bus communications. |
| `sysinfo.c` | System | Displays system hardware specs, CPU clock, memory map, and peripheral status. |
| `testLCD.c` | Diagnostics | Exerciser for the HD44780 2-line LCD display via VIA Port B. |
| `testSID.c` | Multimedia | Sound generator testing SID 3-voice polyphony, ADSR envelopes, and filters. |
| `tetris.c` | Games | Full graphical Tetris game with color blocks, score tracking, and SID sound effects. |
| `touch.c` | File System | Creates empty files or updates file modification timestamps. |
| `via.c` | Diagnostics | Diagnostic tool for testing 6522 VIA timers, interrupts, and shift registers. |
| `view.c` | Multimedia | Displays 100x75 raw graphics binaries (`.bin`) from `/vram/` to the GPU. |
| `wc.c` | Utilities | Word, line, and byte counter for files or piped input. |
| `wget.c` | Networking | HTTP client downloading remote web assets over WiFi directly to the SD card. |
| `wifi.c` | Networking | WiFi configuration tool for scanning SSIDs, connecting, and testing signal. |
| `word.c` | Editors | Advanced TUI word processor with virtual memory swap paging for large documents. |
| `wozmon.c` | System | Embedded Apple 1 Wozniak system monitor CLI running inside microDOS. |

---

## 8. Developer Tutorial: Writing and Packaging a microDOS App

### Application Header Structure
Every `.app` binary begins with a 16-byte base header defined in [`sdk/linker/App-Runtime.s`](file:///home/manu/Documents/GitHub/65c02-SIM/sdk/linker/App-Runtime.s):

```c
struct AppHeader {
    char     magic[4];       // "uDOS"
    uint8_t  version;        // 3
    uint8_t  flags;          // Bit 0: Executable (1)
                             // Bit 1: Has 16x16 icon (2)
                             // Bit 2: No desktop / headless (4)
    uint16_t entry_point;    // Address of _app_startup ($0210 or $0310)
    uint16_t core_size;      // Resident core size in bytes (offset 8)
    uint16_t window_size;    // Overlay window size (0x2000)
    uint16_t window_addr;    // Overlay window load address (0x2800)
    uint16_t num_pages;      // Number of overlay pages or heap start
};
```

If flag bit 1 is set (`HAS_APP_ICON`), a 256-byte array containing a 16x16 icon in RGB 222 format immediately follows the base header at offset 16 (`.header.1_icon`).

### Step-by-Step Tutorial: Creating `hello.app`

#### 1. Write the Application Source Code
Create `sdk/microdos/Apps/hello.c`:

```c
#include "Libs/app-bios.h"

int main(void) {
    char** args = _args_ptr;

    println("=============================");
    println("  Hello from microDOS v7.0!  ");
    println("=============================");

    if (arg_count > 1) {
        print_str("Argument received: ");
        println(args[1]);
    } else {
        println("No arguments provided.");
    }

    print_str("Current working directory: ");
    char cwd[64];
    if (sd_getcwd(cwd, sizeof(cwd))) {
        println(cwd);
    }

    return 0;
}
```

#### 2. Optional: Add a Custom 16x16 Icon
Place any 16x16 image (`hello.png`, `hello.bmp`, or `hello.webp`) in `sdk/microdos/Apps/`. The build script will automatically convert it using [`tools/microDOS/image_to_icon.py`](file:///home/manu/Documents/GitHub/65c02-SIM/tools/microDOS/image_to_icon.py) and link it into `.header.1_icon`.

If the app should not appear in the graphical desktop, add this macro to the top of the file:
```c
MICRODOS_NO_DESKTOP();
```

#### 3. Compile the Application
Use [`sdk/scripts/compile-bin.sh`](file:///home/manu/Documents/GitHub/65c02-SIM/sdk/scripts/compile-bin.sh) with the `--microDOS` flag:

```bash
./sdk/scripts/compile-bin.sh hello --microDOS
```

The build pipeline automatically:
1. Detects included headers (e.g. `--gpu` if `GPU.h` or `Mouse.h` is present).
2. Generates an application-specific linker script via `tools/linker/generate_app_link_ld.py`.
3. Converts the icon image to assembly (`${NAME}_icon.s`) if present.
4. Invokes `mos-clang` with `-mcpu=mosw65c02 -std=c23 -Os -flto` linking with `sdk/linker/App-Runtime.s`.
5. Resolves all Jump Table symbols to `$FF90 + offset`.
6. Generates the final output binary at `output/apps/hello.app`.

#### 4. Run the Application
Copy `output/apps/hello.app` to the `/bin` directory of your SD card image or cartridge, boot microDOS, and launch it from the Desktop or CLI shell:
```bash
uDOS> hello world
=============================
  Hello from microDOS v7.0!  
=============================
Argument received: world
Current working directory: /
```
