#pragma once

#include <imgui.h>

#include "Frontend/Control/AppState.h"

namespace GUI {

using namespace Control;

void DrawVRAMViewerWindow(AppState& state, ImVec2 work_pos, ImVec2 work_size,
                          float top_section_height,
                          ImGuiWindowFlags window_flags);

void ForceRefreshVRAM(AppState& state);

}  // namespace GUI
