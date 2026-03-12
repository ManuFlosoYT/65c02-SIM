#pragma once

#include <imgui.h>

namespace Control {
struct AppState;
}

namespace GUI {
void DrawScriptConsoleWindow(Control::AppState& state, const ImVec2& work_pos, const ImVec2& work_size,
                             float top_section_height, ImGuiWindowFlags window_flags);
}  // namespace GUI
