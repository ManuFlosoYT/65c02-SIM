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
    ImGui::Text("PC: %04X", cpu.PC);
    ImGui::Text("SP: %04X", cpu.SP);
    ImGui::Text("A:  %02X", cpu.A);
    ImGui::Text("X:  %02X", cpu.X);
    ImGui::Text("Y:  %02X", cpu.Y);
    ImGui::Separator();
    ImGui::Text("Flags: %02X", cpu.GetStatus());
    ImGui::Text("N: %d V: %d B: %d D: %d", cpu.N, cpu.V, cpu.B, cpu.D);
    ImGui::Text("I: %d Z: %d C: %d", cpu.I, cpu.Z, cpu.C);
    ImGui::End();
}

}  // namespace GUI
