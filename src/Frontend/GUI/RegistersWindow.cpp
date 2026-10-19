#include "Frontend/GUI/RegistersWindow.h"

#include <format>

#include "Frontend/GUI/Style/Style.h"

using namespace Control;
using namespace Core;
using namespace Hardware;

namespace GUI {

static void DrawFlagBadge(const char* label, bool active) {
    constexpr ImVec2 badgeSize(20.0F, 20.0F);
    constexpr float rounding = 4.0F;

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 p0 = ImGui::GetCursorScreenPos();
    ImVec2 p1 = ImVec2(p0.x + badgeSize.x, p0.y + badgeSize.y);

    ImU32 bgCol = active ? ImGui::GetColorU32(Theme::AccentActive) : ImGui::GetColorU32(Theme::Surface);
    ImU32 textCol = active ? Theme::LcdPetroleumBg : ImGui::GetColorU32(Theme::TextSecondary);

    drawList->AddRectFilled(p0, p1, bgCol, rounding);
    if (!active) {
        drawList->AddRect(p0, p1, ImGui::GetColorU32(Theme::BorderSubtle), rounding);
    }

    ImVec2 textSize = ImGui::CalcTextSize(label);
    ImVec2 textPos(p0.x + ((badgeSize.x - textSize.x) * 0.5F), p0.y + ((badgeSize.y - textSize.y) * 0.5F));
    drawList->AddText(textPos, textCol, label);

    ImGui::Dummy(badgeSize);
}

void DrawRegistersWindow(AppState& state, ImVec2 work_pos, ImVec2 work_size, float top_section_height,
                         ImGuiWindowFlags window_flags) {
    float mainColWidth = work_size.x * 0.47F;
    float lcdHeight = work_size.y * 0.162F;
    float availTop = top_section_height - lcdHeight;
    float controlHeight = std::round(availTop * 0.55F);
    float regHeight = availTop - controlHeight;

    ImGui::SetNextWindowPos(ImVec2(work_pos.x, work_pos.y + controlHeight), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(mainColWidth, regHeight), ImGuiCond_Always);
    ImGui::Begin("Registers", nullptr, window_flags);
    const auto& cpu = state.emulator.GetCPU();
    auto printHex16 = [](const char* prefix, uint16_t val) {
        ImGui::PushStyleColor(ImGuiCol_Text, Theme::TextSecondary);
        ImGui::TextUnformatted(prefix);
        ImGui::PopStyleColor();
        ImGui::SameLine(0.0F, 0.0F);
        std::string valStr = std::format("{:04X}", val);
        ImGui::PushStyleColor(ImGuiCol_Text, Theme::AccentPrimary);
        ImGui::TextUnformatted(valStr.c_str());
        ImGui::PopStyleColor();
    };
    auto printHex8 = [](const char* prefix, uint8_t val) {
        ImGui::PushStyleColor(ImGuiCol_Text, Theme::TextSecondary);
        ImGui::TextUnformatted(prefix);
        ImGui::PopStyleColor();
        ImGui::SameLine(0.0F, 0.0F);
        std::string valStr = std::format("{:02X}", static_cast<int>(val));
        ImGui::PushStyleColor(ImGuiCol_Text, Theme::AccentPrimary);
        ImGui::TextUnformatted(valStr.c_str());
        ImGui::PopStyleColor();
    };

    printHex16("PC: ", cpu.PC);
    ImGui::SameLine(0.0F, 16.0F);
    printHex16("SP: ", cpu.SP);
    ImGui::SameLine(0.0F, 16.0F);
    printHex8("A:  ", cpu.A);
    ImGui::SameLine(0.0F, 16.0F);
    printHex8("X:  ", cpu.X);
    ImGui::SameLine(0.0F, 16.0F);
    printHex8("Y:  ", cpu.Y);

    printHex8("Flags: ", cpu.GetStatus());
    ImGui::SameLine(0.0F, 12.0F);
    DrawFlagBadge("N", cpu.N != 0);
    ImGui::SameLine(0.0F, 4.0F);
    DrawFlagBadge("V", cpu.V != 0);
    ImGui::SameLine(0.0F, 4.0F);
    DrawFlagBadge("B", cpu.B != 0);
    ImGui::SameLine(0.0F, 4.0F);
    DrawFlagBadge("D", cpu.D != 0);
    ImGui::SameLine(0.0F, 4.0F);
    DrawFlagBadge("I", cpu.I != 0);
    ImGui::SameLine(0.0F, 4.0F);
    DrawFlagBadge("Z", cpu.Z != 0);
    ImGui::SameLine(0.0F, 4.0F);
    DrawFlagBadge("C", cpu.C != 0);
    ImGui::End();
}

}  // namespace GUI
