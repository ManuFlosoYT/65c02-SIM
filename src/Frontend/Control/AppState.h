#pragma once

#include <glad/gl.h>

#include <array>
#include <atomic>
#include <cstdint>
#include <string>
#include <vector>

#include "Frontend/Control/EmulationState.h"
#include "Frontend/Control/GLRenderTargets.h"
#include "Frontend/Control/IDEState.h"
#include "Frontend/Control/UIModalState.h"
#include "Frontend/GUI/Video/CRTFilter.h"
#include "Hardware/Core/Emulator/Emulator.h"
#include "Hardware/Core/MemoryScanner.h"
#include "Hardware/Core/SymbolTable.h"

namespace Control {

using namespace Core;
using namespace Hardware;

struct CRTSettings {
    bool scanlines = true;
    bool interlacing = true;
    bool curvature = true;
    bool chromatic = true;
    bool blur = true;
    bool shadowMask = true;
    bool vignette = true;
    bool cornerRounding = true;
    bool glassGlare = true;
    bool colorBleeding = true;
    bool noise = true;
    bool vsyncJitter = true;
    bool phosphorDecay = true;
    bool bloom = true;
    bool ghosting = true;
    bool halation = true;
    bool moire = true;
    bool rasterBeam = false;
    bool sharpPixels = true;
    bool humBar = true;
    GUI::CRTMonitorType monitorType = GUI::CRTMonitorType::RGB;
    float gamma = 2.75F;
    float time = 0.0F;
};

struct AppState {
    Emulator emulator;

    ROMState rom;
    ScriptState script;
    EmulationState emulation;
    DebuggerState debugger;
    UpdateState update;
    CRTSettings crt;
    RenderState render;
    PopupState popups;
    SDKState sdk;
    IDEState ide;
    NetworkSnifferState sniffer;
    ScannerState scanner;
    bool requestExit = false;

    GUI::CRTFilter crtFilter;
};

}  // namespace Control
