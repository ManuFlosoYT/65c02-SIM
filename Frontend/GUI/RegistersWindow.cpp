#include "Frontend/GUI/RegistersWindow.h"

#include <iomanip>
#include <sstream>
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
        std::ostringstream oss;
        oss << prefix << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << val;
        ImGui::TextUnformatted(oss.str().c_str());
    };
    auto printHex8 = [](const char* prefix, uint8_t val) {
        std::ostringstream oss;
        oss << prefix << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << static_cast<int>(val);
        ImGui::TextUnformatted(oss.str().c_str());
    };

    printHex16("PC: ", cpu.PC);
    printHex16("SP: ", cpu.SP);
    printHex8("A:  ", cpu.A);
    printHex8("X:  ", cpu.X);
    printHex8("Y:  ", cpu.Y);
    ImGui::Separator();
    printHex8("Flags: ", cpu.GetStatus());

    std::ostringstream flagsStr1;
    flagsStr1 << "N: " << static_cast<int>(cpu.N) << " V: " << static_cast<int>(cpu.V)
              << " B: " << static_cast<int>(cpu.B) << " D: " << static_cast<int>(cpu.D);
    ImGui::TextUnformatted(flagsStr1.str().c_str());

    std::ostringstream flagsStr2;
    flagsStr2 << "I: " << static_cast<int>(cpu.I) << " Z: " << static_cast<int>(cpu.Z)
              << " C: " << static_cast<int>(cpu.C);
    ImGui::TextUnformatted(flagsStr2.str().c_str());
    ImGui::End();
}

}  // namespace GUI
