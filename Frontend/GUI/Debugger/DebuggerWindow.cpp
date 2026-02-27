#include "Frontend/GUI/Debugger/DebuggerWindow.h"

#include <imgui.h>

#include <iomanip>
#include <random>
#include <sstream>
namespace GUI {

namespace {

void DrawCPURegisters(Hardware::CPU& cpu) {
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7F, 0.7F, 1.0F, 1.0F));
    ImGui::TextUnformatted("CPU Registers");
    ImGui::PopStyleColor();

    ImGui::PushItemWidth(100);

    uint16_t pcVal = cpu.PC;
    if (ImGui::InputScalar("PC", ImGuiDataType_U16, &pcVal, nullptr, nullptr, "%04X",
                           ImGuiInputTextFlags_CharsHexadecimal)) {
        cpu.PC = pcVal;
    }

    uint16_t spVal = cpu.SP;
    if (ImGui::InputScalar("SP", ImGuiDataType_U16, &spVal, nullptr, nullptr, "%04X",
                           ImGuiInputTextFlags_CharsHexadecimal)) {
        cpu.SP = spVal;
    }

    uint8_t aVal = cpu.A;
    if (ImGui::InputScalar("A", ImGuiDataType_U8, &aVal, nullptr, nullptr, "%02X",
                           ImGuiInputTextFlags_CharsHexadecimal)) {
        cpu.A = aVal;
    }

    uint8_t xVal = cpu.X;
    if (ImGui::InputScalar("X", ImGuiDataType_U8, &xVal, nullptr, nullptr, "%02X",
                           ImGuiInputTextFlags_CharsHexadecimal)) {
        cpu.X = xVal;
    }

    uint8_t yVal = cpu.Y;
    if (ImGui::InputScalar("Y", ImGuiDataType_U8, &yVal, nullptr, nullptr, "%02X",
                           ImGuiInputTextFlags_CharsHexadecimal)) {
        cpu.Y = yVal;
    }

    ImGui::PopItemWidth();
}

void DrawMemoryRow(Core::Emulator& emulator, Hardware::Bus& bus, int row) {
    ImGui::TableNextRow();
    ImGui::TableNextColumn();

    std::ostringstream rowBuf;
    rowBuf << std::uppercase << std::setfill('0') << std::setw(4) << std::hex << (row * 16);
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
    ImGui::TextUnformatted(rowBuf.str().c_str());
    ImGui::PopStyleColor();

    for (int col = 0; col < 16; col++) {
        ImGui::TableNextColumn();
        Word addr = static_cast<Word>((row * 16) + col);
        Byte val = bus.ReadDirect(addr);

        std::ostringstream labelBuf;
        labelBuf << "##m" << std::uppercase << std::setfill('0') << std::setw(4) << std::hex << addr;
        std::string labelStr = labelBuf.str();

        ImGui::PushItemWidth(22.0F);
        if (ImGui::InputScalar(labelStr.c_str(), ImGuiDataType_U8, &val, nullptr, nullptr, "%02X",
                               ImGuiInputTextFlags_CharsHexadecimal)) {
            if (addr >= 0x8000) {
                emulator.GetROM().WriteDirect(addr - 0x8000, val);
            } else {
                bus.WriteDirect(addr, val);
            }
        }
        ImGui::PopItemWidth();
    }
}

void DrawMemoryEditor(Core::Emulator& emulator) {
    Hardware::Bus& bus = emulator.GetMem();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7F, 0.7F, 1.0F, 1.0F));
    ImGui::TextUnformatted("Memory Editor (0x0000-0xFFFF)");
    ImGui::PopStyleColor();

    if (ImGui::BeginChild("MemEditScroll", ImVec2(0, -ImGui::GetFrameHeightWithSpacing() * 4.0F),
                          ImGuiChildFlags_Borders)) {
        if (ImGui::BeginTable("MemTable", 17,
                              ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollY)) {
            ImGui::TableSetupColumn("Addr", ImGuiTableColumnFlags_WidthFixed, 45.0F);
            for (int i = 0; i < 16; i++) {
                std::ostringstream buf;
                buf << std::uppercase << std::hex << i;
                ImGui::TableSetupColumn(buf.str().c_str(), ImGuiTableColumnFlags_WidthFixed, 22.0F);
            }
            ImGui::TableHeadersRow();

            ImGuiListClipper clipper;
            clipper.Begin((0xFFFF + 1) / 16);
            while (clipper.Step()) {
                for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++) {
                    DrawMemoryRow(emulator, bus, row);
                }
            }
            ImGui::EndTable();
        }
    }
    ImGui::EndChild();
}

void DrawTools(Core::Emulator& emulator) {
    Hardware::Bus& bus = emulator.GetMem();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7F, 0.7F, 1.0F, 1.0F));
    ImGui::TextUnformatted("Tools");
    ImGui::PopStyleColor();

    static uint16_t startAddr = 0x0000;
    static uint16_t endAddr = 0x00FF;

    ImGui::PushItemWidth(100);
    ImGui::InputScalar("Start", ImGuiDataType_U16, &startAddr, nullptr, nullptr, "%04X",
                       ImGuiInputTextFlags_CharsHexadecimal);
    ImGui::SameLine();
    ImGui::InputScalar("End", ImGuiDataType_U16, &endAddr, nullptr, nullptr, "%04X",
                       ImGuiInputTextFlags_CharsHexadecimal);
    ImGui::PopItemWidth();

    if (ImGui::Button("Fill Random Junk")) {
        std::mt19937 rng(std::random_device{}());
        std::uniform_int_distribution<int> dist(0, 255);
        for (int i = startAddr; i <= static_cast<int>(endAddr); i++) {
            Byte val = static_cast<Byte>(dist(rng));
            Word addr = static_cast<Word>(i);
            if (addr >= 0x8000) {
                emulator.GetROM().WriteDirect(addr - 0x8000, val);
            } else {
                bus.WriteDirect(addr, val);
            }
        }
    }
}

}  // namespace

void DrawDebuggerWindow(Control::AppState& state) {
    Hardware::CPU& cpu = state.emulator.GetCPU();
    Hardware::Bus& bus = state.emulator.GetMem();

    DrawCPURegisters(cpu);

    ImGui::Separator();
    ImGui::Spacing();

    DrawMemoryEditor(state.emulator);

    ImGui::Separator();
    ImGui::Spacing();

    DrawTools(state.emulator);
}

}  // namespace GUI
