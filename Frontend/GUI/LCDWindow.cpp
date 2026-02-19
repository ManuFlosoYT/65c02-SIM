#include "Frontend/GUI/LCDWindow.h"

using namespace Control;
using namespace Core;
using namespace Hardware;

namespace GUI {

void DrawLCDWindow(AppState& state, ImVec2 work_pos, ImVec2 work_size,
                   float top_section_height, ImGuiWindowFlags window_flags) {
    ImGui::SetNextWindowPos(
        ImVec2(work_pos.x, work_pos.y + top_section_height * 0.4f),
        ImGuiCond_Always);
    float lcdWidth = state.emulator.GetSID().IsSoundEnabled()
                         ? work_size.x * 0.4425f
                         : work_size.x * 0.75f;
    ImGui::SetNextWindowSize(ImVec2(lcdWidth, top_section_height * 0.6f),
                             ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 10.0f));
    ImGui::Begin("LCD Output", nullptr, window_flags);

    const auto& lcd = state.emulator.GetLCD();
    const auto& screen = lcd.GetScreen();
    bool is_initialized = lcd.IsInitialized();

    ImU32 cellColor = is_initialized 
        ? IM_COL32(0, 230, 0, 255) 
        : IM_COL32(0, 50, 0, 255);

    ImU32 textColor = IM_COL32(0, 0, 0, 255);

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 p = ImGui::GetCursorScreenPos();

    float avail_w = ImGui::GetContentRegionAvail().x;
    float avail_h = ImGui::GetContentRegionAvail().y;

    float cell_w = avail_w / 16.0f;
    float cell_h = avail_h / 2.0f;
    float padding = 2.0f;

    for (int row = 0; row < 2; ++row) {
        for (int col = 0; col < 16; ++col) {
            float x = p.x + col * cell_w;
            float y = p.y + row * cell_h;

            ImVec2 cell_min = ImVec2(x + padding, y + padding);
            ImVec2 cell_max = ImVec2(x + cell_w - padding, y + cell_h - padding);

            draw_list->AddRectFilled(cell_min, cell_max, cellColor);

            char c = screen[row][col];
            if (c != 0) {
                char str[2] = {c, 0};

                ImFont* font = ImGui::GetFont();
                float fontSize = cell_h * 0.8f;

                ImVec2 textSize =
                    font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, str);
                float textX = x + (cell_w - textSize.x) * 0.5f;
                float textY = y + (cell_h - textSize.y) * 0.5f;

                // Simulating bold text by drawing twice with offset
                draw_list->AddText(font, fontSize, ImVec2(textX + 1.0f, textY),
                                   textColor, str);
                draw_list->AddText(font, fontSize, ImVec2(textX, textY),
                                   textColor, str);
            }
        }
    }

    ImGui::End();
    ImGui::PopStyleVar();
}

}  // namespace GUI
