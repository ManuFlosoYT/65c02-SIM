# Frontend — Graphical User Interface

**Directory:** `Frontend/`  
**Technologies:** ImGui (docking branch) · SDL3 · OpenGL 3.3 · ImGuiFileDialog

## Overview

The frontend provides a **multi-window graphical interface** based on Dear ImGui. It lets you control the emulation, inspect hardware state in real time, and load binary programs. Each hardware component has its own dedicated visualisation window.

## Structure

```
Frontend/
├── main.cpp                  ← Application entry point
├── UpdateChecker.h/cpp       ← Update checker (HTTPS)
├── Control/
│   ├── AppState.h            ← Global application state
│   └── Console.h/cpp         ← Console text buffer
└── GUI/
    ├── ControlWindow         ← Main controls
    ├── ConsoleWindow         ← Text console
    ├── RegistersWindow       ← CPU register viewer
    ├── LCDWindow             ← LCD display
    ├── VRAMViewerWindow      ← VRAM viewer
    ├── SIDViewerWindow       ← SID chip viewer
    └── UpdatePopup           ← Update available popup
```

## GUI windows

### ControlWindow — Emulation controls

Allows the user to:
- **Load a binary** using the file dialog (ImGuiFileDialog)
- **Start / Pause / Resume / Stop** emulation
- **Step** — execute a single instruction
- Configure the **speed** (Hz / IPS — instructions per second)
- Toggle **cycle-accurate mode**
- Toggle the **GPU**
- Display the **current speed** (actual Hz / IPS)

### ConsoleWindow — Text console

Displays text sent by the ACIA (serial port of the 65c02). Also lets you **type** text that is injected as input to the emulator (`InjectKey`).

Features:
- Circular text buffer
- Auto-scroll to bottom
- Copy to clipboard

### RegistersWindow — Register viewer

Shows the CPU state in real time:

| Field | Description |
|-------|-------------|
| `PC` | Program Counter (hex) |
| `SP` | Stack Pointer (hex) |
| `A` | Accumulator |
| `X` | X register |
| `Y` | Y register |
| `N V - B D I Z C` | Individual status flags |
| Remaining cycles | In cycle-accurate mode |

### LCDWindow — LCD display

Renders the current contents of the 2×16 LCD with a monospace font, simulating a physical LCD screen.

### VRAMViewerWindow — VRAM viewer

Renders the VRAM contents (100×75 pixels) as a scaled OpenGL texture. Updated every renderer frame.

### SIDViewerWindow — SID viewer

Shows the state of all 3 SID chip oscillators:
- Programmed frequency for each voice
- Pulse width
- ADSR envelope state (level bar)
- Active waveforms (Triangle / Sawtooth / Pulse / Noise)

### UpdatePopup — Update notification

At startup, `UpdateChecker` queries the latest version on GitHub via HTTPS. If a newer version is available, a popup is shown with a link to the releases page.

```cpp
UpdateChecker checker;
checker.CheckAsync([](const std::string& latestVersion) {
    // Show popup if latestVersion > currentVersion
});
```

## AppState — Global state

`Control/AppState.h` defines the shared state structure between the GUI and the control layer. It holds references to the emulator, status flags (paused, running, etc.), and the path of the loaded binary.

## Render loop

The main loop (`main.cpp`) follows the standard ImGui + SDL3 + OpenGL pattern:

```
while (running):
    1. Process SDL3 events (keyboard, mouse, window close)
    2. ImGui::NewFrame()
    3. Render all windows (ControlWindow, ConsoleWindow, ...)
    4. ImGui::Render()
    5. glClear + ImGui_ImplOpenGL3_RenderDrawData()
    6. SDL_GL_SwapWindow()
```

## External dependencies

| Library | Version | Use |
|---------|---------|-----|
| SDL3 | 3.4.x | Window, events, audio |
| ImGui | docking branch | GUI framework |
| ImGuiFileDialog | 0.6.4 | File open dialog |
| GLAD | 2.0.8 | OpenGL loader |
| nlohmann/json | 3.12.0 | JSON parsing (GitHub API response) |
| cpp-httplib | 0.15.3 | HTTP/HTTPS client for update checker |
| OpenSSL | — | TLS for HTTPS requests |
