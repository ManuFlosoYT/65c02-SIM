#include "Frontend/GUI/LCDWindow.h"

#include <cstring>

namespace GUI {

void DrawLCDWindow(AppState& state, ImVec2 work_pos, ImVec2 work_size,
                   float top_section_height, ImGuiWindowFlags window_flags) {
    ImGui::SetNextWindowPos(
        ImVec2(work_pos.x, work_pos.y + top_section_height * 0.4f),
        ImGuiCond_Always);
    float lcdWidth = state.emulator.GetSID().IsSoundEnabled()
                         ? work_size.x * 0.375f
                         : work_size.x * 0.75f;
    ImGui::SetNextWindowSize(ImVec2(lcdWidth, top_section_height * 0.6f),
                             ImGuiCond_Always);
    ImGui::Begin("LCD Output", nullptr, window_flags);

    const auto& screen = state.emulator.GetLCDScreen();
    char line1[17];
    char line2[17];
    std::memcpy(line1, screen[0], 16);
    line1[16] = 0;
    std::memcpy(line2, screen[1], 16);
    line2[16] = 0;
    ImGui::Text("%s", line1);
    ImGui::Text("%s", line2);
    ImGui::End();
}

}  // namespace GUI
