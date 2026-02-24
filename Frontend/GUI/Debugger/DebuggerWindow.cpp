#include "Frontend/GUI/Debugger/DebuggerWindow.h"

#include <imgui.h>

#include <cstdlib>

namespace GUI {

void DrawDebuggerWindow(Control::AppState& state) {
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 1.0f, 1.0f), "CPU Registers");
    Hardware::CPU& cpu = state.emulator.GetCPU();
    ImGui::PushItemWidth(100);

    uint16_t pcVal = cpu.PC;
    if (ImGui::InputScalar("PC", ImGuiDataType_U16, &pcVal, NULL, NULL, "%04X",
                           ImGuiInputTextFlags_CharsHexadecimal)) {
        cpu.PC = pcVal;
    }

    uint16_t spVal = cpu.SP;
    if (ImGui::InputScalar("SP", ImGuiDataType_U16, &spVal, NULL, NULL, "%04X",
                           ImGuiInputTextFlags_CharsHexadecimal)) {
        cpu.SP = spVal;
    }

    uint8_t aVal = cpu.A;
    if (ImGui::InputScalar("A", ImGuiDataType_U8, &aVal, NULL, NULL, "%02X",
                           ImGuiInputTextFlags_CharsHexadecimal)) {
        cpu.A = aVal;
    }

    uint8_t xVal = cpu.X;
    if (ImGui::InputScalar("X", ImGuiDataType_U8, &xVal, NULL, NULL, "%02X",
                           ImGuiInputTextFlags_CharsHexadecimal)) {
        cpu.X = xVal;
    }

    uint8_t yVal = cpu.Y;
    if (ImGui::InputScalar("Y", ImGuiDataType_U8, &yVal, NULL, NULL, "%02X",
                           ImGuiInputTextFlags_CharsHexadecimal)) {
        cpu.Y = yVal;
    }

    ImGui::PopItemWidth();

    ImGui::Separator();
    ImGui::Spacing();

    ImGui::TextColored(ImVec4(0.7f, 0.7f, 1.0f, 1.0f),
                       "Memory Editor (0x0000-0xFFFF)");
    Hardware::Mem& mem = state.emulator.GetMem();

    if (ImGui::BeginChild("MemEditScroll",
                          ImVec2(0, -ImGui::GetFrameHeightWithSpacing() * 4),
                          true)) {
        if (ImGui::BeginTable("MemTable", 17,
                              ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders |
                                  ImGuiTableFlags_ScrollY)) {
            ImGui::TableSetupColumn("Addr", ImGuiTableColumnFlags_WidthFixed,
                                    45.0f);
            for (int i = 0; i < 16; i++) {
                char buf[4];
                sprintf(buf, "%01X", i);
                ImGui::TableSetupColumn(buf, ImGuiTableColumnFlags_WidthFixed,
                                        22.0f);
            }
            ImGui::TableHeadersRow();

            ImGuiListClipper clipper;
            clipper.Begin((0xFFFF + 1) / 16);
            while (clipper.Step()) {
                for (int row = clipper.DisplayStart; row < clipper.DisplayEnd;
                     row++) {
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::TextDisabled("%04X", row * 16);

                    for (int col = 0; col < 16; col++) {
                        ImGui::TableNextColumn();
                        Word addr = (Word)(row * 16 + col);
                        Byte val = mem.Peek(addr);

                        char label[16];
                        sprintf(label, "##m%04X", addr);
                        ImGui::PushItemWidth(22.0f);
                        if (ImGui::InputScalar(
                                label, ImGuiDataType_U8, &val, NULL, NULL,
                                "%02X", ImGuiInputTextFlags_CharsHexadecimal)) {
                            mem.WriteDebug(addr, val);
                        }
                        ImGui::PopItemWidth();
                    }
                }
            }
            ImGui::EndTable();
        }
    }
    ImGui::EndChild();

    ImGui::Separator();
    ImGui::Spacing();

    ImGui::TextColored(ImVec4(0.7f, 0.7f, 1.0f, 1.0f), "Tools");
    static uint16_t startAddr = 0x0000;
    static uint16_t endAddr = 0x00FF;

    ImGui::PushItemWidth(100);
    ImGui::InputScalar("Start", ImGuiDataType_U16, &startAddr, NULL, NULL,
                       "%04X", ImGuiInputTextFlags_CharsHexadecimal);
    ImGui::SameLine();
    ImGui::InputScalar("End", ImGuiDataType_U16, &endAddr, NULL, NULL, "%04X",
                       ImGuiInputTextFlags_CharsHexadecimal);
    ImGui::PopItemWidth();

    if (ImGui::Button("Fill Random Junk")) {
        for (int i = startAddr; i <= (int)endAddr; i++) {
            mem.WriteDebug((Word)i, (Byte)(rand() % 256));
        }
    }
}

}  // namespace GUI
