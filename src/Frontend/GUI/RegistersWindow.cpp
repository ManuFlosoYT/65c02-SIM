#include "Frontend/GUI/RegistersWindow.h"

using namespace Control;
using namespace Core;
using namespace Hardware;

namespace GUI {

void DrawRegistersWindow(AppState& state, ImVec2 work_pos, ImVec2 work_size, float top_section_height,
                         ImGuiWindowFlags window_flags) {
    ImGui::SetNextWindowPos(ImVec2(work_pos.x + (work_size.x * 0.80F), work_pos.y), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(work_size.x * 0.20F, top_section_height), ImGuiCond_Always);
    ImGui::Begin("Registers", nullptr, window_flags);
    const auto& cpu = state.emulator.GetCPU();
    auto printHex16 = [](const char* prefix, uint16_t val) {
        ImGui::Text("%s%04X", prefix, val);
    };
    auto printHex8 = [](const char* prefix, uint8_t val) {
        ImGui::Text("%s%02X", prefix, static_cast<int>(val));
    };

    printHex16("PC: ", cpu.PC);
    printHex16("SP: ", cpu.SP);
    printHex8("A:  ", cpu.A);
    printHex8("X:  ", cpu.X);
    printHex8("Y:  ", cpu.Y);
    ImGui::Separator();
    printHex8("Flags: ", cpu.GetStatus());

    ImGui::Text("N: %d V: %d B: %d D: %d", static_cast<int>(cpu.N), static_cast<int>(cpu.V),
                static_cast<int>(cpu.B), static_cast<int>(cpu.D));
    ImGui::Text("I: %d Z: %d C: %d", static_cast<int>(cpu.I), static_cast<int>(cpu.Z),
                static_cast<int>(cpu.C));
    ImGui::End();
}

}  // namespace GUI
