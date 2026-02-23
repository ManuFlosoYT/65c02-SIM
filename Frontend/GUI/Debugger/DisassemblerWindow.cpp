#include "Frontend/GUI/Debugger/DisassemblerWindow.h"

#include <imgui.h>

#include "Hardware/CPU/Disassembler.h"

namespace GUI {

void DrawDisassemblerContent(Control::AppState& state) {
    ImGui::BeginChild("DisassemblyScroll", ImVec2(0, 0), true);

    if (ImGui::BeginTable("DisassemblyTable", 4,
                          ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY |
                              ImGuiTableFlags_BordersOuter |
                              ImGuiTableFlags_BordersV)) {
        ImGui::TableSetupColumn("Address", ImGuiTableColumnFlags_WidthFixed,
                                80.0f);
        ImGui::TableSetupColumn("Bytes", ImGuiTableColumnFlags_WidthFixed,
                                80.0f);
        ImGui::TableSetupColumn("Mnemonic", ImGuiTableColumnFlags_WidthFixed,
                                80.0f);
        ImGui::TableSetupColumn("Operands", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();

        uint16_t pc = state.emulator.GetCPU().PC;
        // Search back a few bytes to find a potential start point
        // This is heuristic, but for short ranges it usually works
        uint16_t currentAddr = (pc > 32) ? pc - 32 : 0;

        // Dissassemble a block around PC
        for (int i = 0; i < 50; i++) {
            Hardware::DisassembledInstruction di =
                Hardware::Disassembler::Disassemble(state.emulator.GetMem(),
                                                    currentAddr);

            ImGui::TableNextRow();
            if (currentAddr == pc) {
                ImGui::TableSetBgColor(
                    ImGuiTableBgTarget_RowBg0,
                    ImGui::GetColorU32(ImVec4(0.3f, 0.3f, 0.7f, 0.6f)));
            }

            ImGui::TableNextColumn();
            ImGui::Text("%s", di.address.c_str());

            ImGui::TableNextColumn();
            ImGui::Text("%s", di.bytes.c_str());

            ImGui::TableNextColumn();
            ImGui::Text("%s", di.mnemonic.c_str());

            ImGui::TableNextColumn();
            ImGui::Text("%s", di.operands.c_str());

            currentAddr += di.size;
            if (currentAddr < di.size) break;  // Overflow
        }
        ImGui::EndTable();
    }

    ImGui::EndChild();
}

}  // namespace GUI
