#pragma once

#include "Frontend/Control/AppState.h"
#include "Hardware/Video/GPU.h"
#include <cstdint>
#include <string>

namespace GUI {

class VRAMImageIO {
public:
    static void ConvertVRAMToRGB(const Hardware::GPU& gpu, uint8_t* dst);
    static void LoadVRAMFromFile(const std::string& imgPath, Control::AppState& state);
    static void ExportVRAMBMP(Control::AppState& state);
#ifndef TARGET_WASM
    static void HandleVRAMLoadDialog(Control::AppState& state);
    static void HandleVRAMSaveDialog(Control::AppState& state);
#endif
};

}  // namespace GUI
