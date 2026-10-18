#include "Frontend/GUI/Debugger/DebuggerWindow.h"

#include <imgui.h>

#include <array>
#include <random>
namespace GUI {

namespace {

void DrawCPURegisters(Control::AppState& state) {
    Hardware::CPU& cpu = state.emulator.GetCPU();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7F, 0.7F, 1.0F, 1.0F));
    ImGui::TextUnformatted("CPU Registers");
    ImGui::PopStyleColor();

    ImGui::PushItemWidth(100);

    uint16_t pcVal = cpu.PC;
    if (ImGui::InputScalar("PC", ImGuiDataType_U16, &pcVal, nullptr, nullptr, "%04X",
                           ImGuiInputTextFlags_CharsHexadecimal)) {
        state.emulator.WaitUntilSafeToMutate();
        cpu.PC = static_cast<uint16_t>(pcVal);
        cpu.UpdatePagePtr(state.emulator.GetMem());
    }

    uint16_t spVal = cpu.SP;
    if (ImGui::InputScalar("SP", ImGuiDataType_U16, &spVal, nullptr, nullptr, "%04X",
                           ImGuiInputTextFlags_CharsHexadecimal)) {
        state.emulator.WaitUntilSafeToMutate();
        cpu.SP = spVal;
    }

    uint8_t aVal = cpu.A;
    if (ImGui::InputScalar("A", ImGuiDataType_U8, &aVal, nullptr, nullptr, "%02X",
                           ImGuiInputTextFlags_CharsHexadecimal)) {
        state.emulator.WaitUntilSafeToMutate();
        cpu.A = aVal;
    }

    uint8_t xVal = cpu.X;
    if (ImGui::InputScalar("X", ImGuiDataType_U8, &xVal, nullptr, nullptr, "%02X",
                           ImGuiInputTextFlags_CharsHexadecimal)) {
        state.emulator.WaitUntilSafeToMutate();
        cpu.X = xVal;
    }

    uint8_t yVal = cpu.Y;
    if (ImGui::InputScalar("Y", ImGuiDataType_U8, &yVal, nullptr, nullptr, "%02X",
                           ImGuiInputTextFlags_CharsHexadecimal)) {
        state.emulator.WaitUntilSafeToMutate();
        cpu.Y = yVal;
    }

    ImGui::PopItemWidth();
}

void DrawMemoryRow(Hardware::Bus& bus, int row) {
    ImGui::TableNextRow();
    ImGui::TableNextColumn();

    ImGui::TextDisabled("%04X", row * 16);

    for (int col = 0; col < 16; col++) {
        ImGui::TableNextColumn();
        Word addr = static_cast<Word>((row * 16) + col);
        Byte val = bus.ReadDirect(addr);

        ImGui::PushID(addr);
        ImGui::PushItemWidth(24.2F);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.0F, ImGui::GetStyle().FramePadding.y));
        if (ImGui::InputScalar("##m", ImGuiDataType_U8, &val, nullptr, nullptr, "%02X",
                               ImGuiInputTextFlags_CharsHexadecimal)) {
            bus.WriteDirect(addr, val);
        }
        ImGui::PopStyleVar();
        ImGui::PopItemWidth();
        ImGui::PopID();
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
            static constexpr std::array<const char*, 16> kHexHeaders = {
                "0", "1", "2", "3", "4", "5", "6", "7",
                "8", "9", "A", "B", "C", "D", "E", "F"
            };
            for (const char* header : kHexHeaders) {
                ImGui::TableSetupColumn(header, ImGuiTableColumnFlags_WidthFixed, 24.2F);
            }
            ImGui::TableHeadersRow();

            ImGuiListClipper clipper;
            clipper.Begin((0xFFFF + 1) / 16);
            while (clipper.Step()) {
                for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++) {
                    DrawMemoryRow(bus, row);
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
    ImGui::SameLine();
    if (ImGui::Button("Fill Random Junk")) {
        std::mt19937 rng(std::random_device{}());
        std::uniform_int_distribution<int> dist(0, 255);
        for (int i = startAddr; i <= static_cast<int>(endAddr); i++) {
            Byte val = static_cast<Byte>(dist(rng));
            Word addr = static_cast<Word>(i);
            bus.WriteDirect(addr, val);
        }
    }

    if (ImGui::Button("Corrupt Random Byte")) {
        std::mt19937 rng(std::random_device{}());
        std::uniform_int_distribution<int> addrDist(0, 0xFFFF);
        std::uniform_int_distribution<int> valDist(0, 0xFF);

        Word addr = static_cast<Word>(addrDist(rng));
        Byte val = static_cast<Byte>(valDist(rng));

        bus.WriteDirect(addr, val);
    }
}

}  // namespace

void DrawDebuggerWindow(Control::AppState& state) {
    DrawCPURegisters(state);

    ImGui::Separator();
    ImGui::Spacing();

    DrawMemoryEditor(state.emulator);

    ImGui::Separator();
    ImGui::Spacing();

    DrawTools(state.emulator);
}

}  // namespace GUI
