#include "Frontend/GUI/Debugger/DebuggerWindow.h"

#include <imgui.h>

#include <algorithm>
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

void DrawMemoryRow(Core::Emulator& emulator, Hardware::Bus& bus, int row, uint16_t pc, bool heatmapEnabled) {
    ImGui::TableNextRow();
    ImGui::TableNextColumn();

    ImGui::TextDisabled("%04X", row * 16);

    const ImVec4 baseFrameBg = ImGui::GetStyleColorVec4(ImGuiCol_FrameBg);

    for (int col = 0; col < 16; col++) {
        ImGui::TableNextColumn();
        Word addr = static_cast<Word>((row * 16) + col);
        Byte val = bus.ReadDirect(addr);

        int colorsPushed = 0;
        if (heatmapEnabled) {
            if (addr == pc) {
                ImVec4 pcCol(0.12F, 0.70F, 0.18F, 0.85F);
                ImGui::PushStyleColor(ImGuiCol_FrameBg, pcCol);
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0F, 1.0F, 1.0F, 1.0F));
                ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, ImGui::GetColorU32(pcCol));
                colorsPushed = 2;
            } else {
                uint8_t w = bus.GetWriteHeat(addr);
                uint8_t r = bus.GetReadHeat(addr);
                if (w > 0 || r > 0) {
                    float rw = static_cast<float>(w) / 255.0F;
                    float rb = static_cast<float>(r) / 255.0F;
                    float t = std::max(rw, rb);
                    float red = baseFrameBg.x * (1.0F - rw) + 0.90F * rw;
                    float green = baseFrameBg.y * (1.0F - t) + 0.10F * t;
                    float blue = baseFrameBg.z * (1.0F - rb) + 0.95F * rb;
                    float alpha = baseFrameBg.w * (1.0F - t) + 0.90F * t;
                    ImVec4 heatCol(red, green, blue, alpha);
                    ImGui::PushStyleColor(ImGuiCol_FrameBg, heatCol);
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0F, 1.0F, 1.0F, 1.0F));
                    ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, ImGui::GetColorU32(heatCol));
                    colorsPushed = 2;
                }
            }
        }

        ImGui::PushID(addr);
        ImGui::PushItemWidth(24.2F);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.0F, ImGui::GetStyle().FramePadding.y));
        if (ImGui::InputScalar("##m", ImGuiDataType_U8, &val, nullptr, nullptr, "%02X",
                               ImGuiInputTextFlags_CharsHexadecimal)) {
            emulator.WaitUntilSafeToMutate();
            bus.WriteDirect(addr, val);
        }
        ImGui::PopStyleVar();
        ImGui::PopItemWidth();
        ImGui::PopID();

        if (colorsPushed > 0) {
            ImGui::PopStyleColor(colorsPushed);
        }
    }
}

void DrawMemoryEditor(Control::AppState& state) {
    Hardware::Bus& bus = state.emulator.GetMem();
    if (!state.emulator.IsPaused() && state.debugger.heatmapEnabled) {
        float dt = ImGui::GetIO().DeltaTime;
        auto decay = static_cast<uint8_t>(std::clamp(static_cast<int>(255.0F * dt / 0.8F), 1, 255));
        bus.DecayHeatmap(decay);
    }

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7F, 0.7F, 1.0F, 1.0F));
    ImGui::TextUnformatted("Memory Editor (0x0000-0xFFFF)");
    ImGui::PopStyleColor();
    ImGui::SameLine();
    ImGui::Checkbox("Heatmap", &state.debugger.heatmapEnabled);
    if (state.debugger.heatmapEnabled) {
        ImGui::SameLine();
        ImGui::TextDisabled("|");
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.2F, 0.9F, 0.3F, 1.0F), "PC");
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.9F, 0.3F, 0.3F, 1.0F), "Write");
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.3F, 0.6F, 1.0F, 1.0F), "Read");
    }

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

            uint16_t pc = state.emulator.GetCPU().PC;
            bool heatmapEnabled = state.debugger.heatmapEnabled;
            ImGuiListClipper clipper;
            clipper.Begin((0xFFFF + 1) / 16);
            while (clipper.Step()) {
                for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++) {
                    DrawMemoryRow(state.emulator, bus, row, pc, heatmapEnabled);
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
        emulator.WaitUntilSafeToMutate();
        std::mt19937 rng(std::random_device{}());
        std::uniform_int_distribution<int> dist(0, 255);
        for (int i = startAddr; i <= static_cast<int>(endAddr); i++) {
            Byte val = static_cast<Byte>(dist(rng));
            Word addr = static_cast<Word>(i);
            bus.WriteDirect(addr, val);
        }
    }

    if (ImGui::Button("Corrupt Random Byte")) {
        emulator.WaitUntilSafeToMutate();
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

    DrawMemoryEditor(state);

    ImGui::Separator();
    ImGui::Spacing();

    DrawTools(state.emulator);
}

}  // namespace GUI
