#include "Frontend/GUI/Debugger/DisassemblerWindow.h"

#include <imgui.h>

#include "Hardware/CPU/Disassembler.h"

namespace GUI {

void DrawDisassemblerContent(Control::AppState& state) {
    ImGui::BeginChild("DisassemblyScroll", ImVec2(0, 0), ImGuiChildFlags_Borders);

    if (ImGui::BeginTable("DisassemblyTable", 4,
                          ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_BordersOuter |
                              ImGuiTableFlags_BordersV)) {
        ImGui::TableSetupColumn("Address", ImGuiTableColumnFlags_WidthFixed, 80.0F);
        ImGui::TableSetupColumn("Bytes", ImGuiTableColumnFlags_WidthFixed, 80.0F);
        ImGui::TableSetupColumn("Mnemonic", ImGuiTableColumnFlags_WidthFixed, 80.0F);
        ImGui::TableSetupColumn("Operands", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();

        uint16_t progCounter = state.emulator.GetCPU().PC;
        // Search back a few bytes to find a potential start point
        // This is heuristic, but for short ranges it usually works
        uint16_t currentAddr = (progCounter > 32) ? progCounter - 32 : 0;

        // Dissassemble a block around PC
        for (int i = 0; i < 50; i++) {
            Hardware::DisassembledInstruction instr =
                Hardware::Disassembler::Disassemble(state.emulator.GetMem(), currentAddr);

            ImGui::TableNextRow();
            if (currentAddr == progCounter) {
                ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, ImGui::GetColorU32(ImVec4(0.3F, 0.3F, 0.7F, 0.6F)));
            }

            ImGui::TableNextColumn();
            ImGui::TextUnformatted(instr.address.c_str());

            ImGui::TableNextColumn();
            ImGui::TextUnformatted(instr.bytes.c_str());

            ImGui::TableNextColumn();
            ImGui::TextUnformatted(instr.mnemonic.c_str());

            ImGui::TableNextColumn();
            ImGui::TextUnformatted(instr.operands.c_str());

            currentAddr += instr.size;
            if (currentAddr < instr.size) {
                break;  // Overflow
            }
        }
        ImGui::EndTable();
    }

    ImGui::EndChild();
}

}  // namespace GUI
