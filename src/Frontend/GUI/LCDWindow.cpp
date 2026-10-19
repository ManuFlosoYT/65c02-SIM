#include "Frontend/GUI/LCDWindow.h"
#include "Frontend/GUI/Style/Style.h"

using namespace Control;
using namespace Core;
using namespace Hardware;

namespace GUI {

void DrawLCDWindow(AppState& state, ImVec2 work_pos, ImVec2 work_size, float top_section_height,
                   ImGuiWindowFlags window_flags) {
    float mainColWidth = work_size.x * 0.47F;
    float lcdHeight = work_size.y * 0.162F;
    float controlAndRegHeight = top_section_height - lcdHeight;
    ImGui::SetNextWindowPos(ImVec2(work_pos.x, work_pos.y + controlAndRegHeight), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(mainColWidth, lcdHeight), ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0F, 10.0F));
    ImGui::Begin("LCD Output", nullptr, window_flags);

    const auto& lcd = state.emulator.GetLCD();
    const auto& screen = lcd.GetScreen();
    bool is_initialized = lcd.IsInitialized();

    ImU32 cellColor = is_initialized ? Theme::LcdCellInactive : Theme::LcdPetroleumBg;

    ImU32 textColor = Theme::LcdActivePixel;

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 cursorPos = ImGui::GetCursorScreenPos();

    float avail_w = ImGui::GetContentRegionAvail().x;
    float avail_h = ImGui::GetContentRegionAvail().y;

    draw_list->AddRectFilled(cursorPos, ImVec2(cursorPos.x + avail_w, cursorPos.y + avail_h), Theme::LcdPetroleumBg);
    draw_list->AddRect(cursorPos, ImVec2(cursorPos.x + avail_w, cursorPos.y + avail_h), Theme::LcdBevelBorder);

    float cell_w = avail_w / 16.0F;
    float cell_h = avail_h / 2.0F;
    float padding = 2.0F;

    for (int row = 0; row < 2; ++row) {
        for (int col = 0; col < 16; ++col) {
            float charX = cursorPos.x + ((float)col * cell_w);
            float charY = cursorPos.y + ((float)row * cell_h);

            ImVec2 cell_min = ImVec2(charX + padding, charY + padding);
            ImVec2 cell_max = ImVec2(charX + cell_w - padding, charY + cell_h - padding);

            draw_list->AddRectFilled(cell_min, cell_max, cellColor);

            if (lcd.IsDisplayOn()) {
                char charChar = screen.at(row).at(col);
                if (charChar != 0) {
                    std::array<char, 2> str = {charChar, 0};

                    ImFont* font = ImGui::GetFont();
                    float fontSize = cell_h * 0.8F;

                    ImVec2 textSize = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0F, str.data());
                    float textX = charX + ((cell_w - textSize.x) * 0.5F);
                    float textY = charY + ((cell_h - textSize.y) * 0.5F);

                    // Simulating bold text by drawing twice with offset
                    draw_list->AddText(font, fontSize, ImVec2(textX + 1.0F, textY), textColor, str.data());
                    draw_list->AddText(font, fontSize, ImVec2(textX, textY), textColor, str.data());
                }

                if (lcd.IsCursorOn() && row == lcd.GetCursorY() && col == lcd.GetCursorX()) {
                    ImVec2 cursor_min = ImVec2(cell_min.x + 2.0F, cell_max.y - (cell_h * 0.15F));
                    ImVec2 cursor_max = ImVec2(cell_max.x - 2.0F, cell_max.y - 2.0F);
                    draw_list->AddRectFilled(cursor_min, cursor_max, textColor);
                }
            }
        }
    }

    ImGui::End();
    ImGui::PopStyleVar();
}

}  // namespace GUI
