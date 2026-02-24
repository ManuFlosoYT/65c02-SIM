#include "Frontend/GUI/Debugger/DebuggerWindow.h"

#include <ImGuiFileDialog.h>
#include <imgui.h>

#include <cstdlib>
#include <ctime>

#include "Frontend/Control/Console.h"
#include "Frontend/GUI/Debugger/DisassemblerWindow.h"
#include "Frontend/GUI/Debugger/ProfilerWindow.h"

namespace GUI {

void DrawDebuggerWindow(Control::AppState& state) {
    if (!state.debuggerOpen) {
        state.emulator.SetProfilingEnabled(false);
        return;
    }

    state.emulator.SetProfilingEnabled(true);

    ImVec2 vpSize = ImGui::GetMainViewport()->Size;
    ImGui::SetNextWindowSize(ImVec2(vpSize.x * 0.7f, vpSize.y * 0.7f),
                             ImGuiCond_FirstUseEver);
    if (!ImGui::Begin(
            "Debugger", &state.debuggerOpen,
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize)) {
        state.emulator.SetProfilingEnabled(false);
        ImGui::End();
        return;
    }

    // Left sidebar
    const char* modes[] = {"Disassembly", "Profiler", "Debugger"};
    const float sidebarWidth = 140.0f;
    ImGui::BeginChild("DebugSidebar", ImVec2(sidebarWidth, 0), false,
                      ImGuiWindowFlags_NoScrollbar);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 6));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 2));
    for (int i = 0; i < 3; i++) {
        bool selected = (state.debuggerMode == i);
        if (selected) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(
                                                       ImGuiCol_ButtonActive));
        } else {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        }
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                              ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered));
        if (ImGui::Button(modes[i], ImVec2(sidebarWidth, 0)))
            state.debuggerMode = i;
        ImGui::PopStyleColor(2);
    }
    ImGui::PopStyleVar(2);

    // Control buttons anchored to bottom of sidebar
    float buttonHeight = ImGui::GetFrameHeightWithSpacing();
    float totalBottomSpace = (buttonHeight * 6.0f);
    float horizontalPadding = 5.0f;
    float adjustedWidth = sidebarWidth - (horizontalPadding * 2.0f);

    ImGui::SetCursorPosY(ImGui::GetWindowHeight() - totalBottomSpace);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 6));

    // Run/Pause
    ImGui::BeginDisabled(!state.romLoaded);
    ImGui::SetCursorPosX(horizontalPadding);
    if (ImGui::Button(state.emulator.IsPaused() ? "Run " : "Pause",
                      ImVec2(adjustedWidth, 0))) {
        if (state.emulator.IsPaused())
            state.emulator.Resume();
        else
            state.emulator.Pause();
        state.emulator.GetSID().SetEmulationPaused(state.emulator.IsPaused());
    }

    // Step
    ImGui::SetCursorPosX(horizontalPadding);
    if (ImGui::Button("Step", ImVec2(adjustedWidth, 0))) {
        state.emulator.GetSID().SetEmulationPaused(false);
        state.emulator.Pause();
        state.emulator.Step();
        state.emulator.GetSID().SetEmulationPaused(true);
    }
    ImGui::EndDisabled();

    // Reset
    ImGui::SetCursorPosX(horizontalPadding);
    if (ImGui::Button("Reset", ImVec2(adjustedWidth, 0))) {
        bool wasRunning = !state.emulator.IsPaused();
        if (wasRunning) state.emulator.Pause();
        Console::Clear();
        state.emulator.GetGPU().Init();
        state.emulator.ClearProfiler();
        if (state.romLoaded) {
            std::string errorMsg;
            if (!state.emulator.Init(state.bin, errorMsg)) {
                state.romLoaded = false;
                ImGuiFileDialog::Instance()->OpenDialog(
                    "ChooseFileDlgKey", "Choose File", ".bin", ".");
            } else {
                state.emulator.SetGPUEnabled(state.gpuEnabled);
            }
        }
        if (wasRunning) {
            state.emulator.Resume();
            state.emulator.GetSID().SetEmulationPaused(false);
        }
    }

    ImGui::Spacing();

    // Exit
    ImGui::SetCursorPosX(horizontalPadding);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                          ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered));
    if (ImGui::Button("Exit", ImVec2(adjustedWidth, 0)))
        state.debuggerOpen = false;
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar();
    ImGui::EndChild();

    ImGui::SameLine();

    // Vertical separator
    float lineX =
        ImGui::GetCursorScreenPos().x - ImGui::GetStyle().ItemSpacing.x;
    ImGui::GetWindowDrawList()->AddLine(
        ImVec2(lineX, ImGui::GetWindowPos().y),
        ImVec2(lineX, ImGui::GetWindowPos().y + ImGui::GetWindowHeight()),
        ImGui::GetColorU32(ImGuiCol_Separator), 1.0f);

    // Content area
    ImGui::BeginChild("DebugContent", ImVec2(0, 0), false);

    if (state.debuggerMode == 0) {  // Disassembly
        DrawDisassemblerContent(state);
    } else if (state.debuggerMode == 1) {  // Profiler
        DrawProfilerWindow(state);
    } else {  // Debugger
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 1.0f, 1.0f), "CPU Registers");
        Hardware::CPU& cpu = state.emulator.GetCPU();
        ImGui::PushItemWidth(100);

        uint16_t pcVal = cpu.PC;
        if (ImGui::InputScalar("PC", ImGuiDataType_U16, &pcVal, NULL, NULL,
                               "%04X", ImGuiInputTextFlags_CharsHexadecimal)) {
            cpu.PC = pcVal;
        }

        uint16_t spVal = cpu.SP;
        if (ImGui::InputScalar("SP", ImGuiDataType_U16, &spVal, NULL, NULL,
                               "%04X", ImGuiInputTextFlags_CharsHexadecimal)) {
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

        if (ImGui::BeginChild(
                "MemEditScroll",
                ImVec2(0, -ImGui::GetFrameHeightWithSpacing() * 4), true)) {
            if (ImGui::BeginTable("MemTable", 17,
                                  ImGuiTableFlags_RowBg |
                                      ImGuiTableFlags_Borders |
                                      ImGuiTableFlags_ScrollY)) {
                ImGui::TableSetupColumn(
                    "Addr", ImGuiTableColumnFlags_WidthFixed, 45.0f);
                for (int i = 0; i < 16; i++) {
                    char buf[4];
                    sprintf(buf, "%01X", i);
                    ImGui::TableSetupColumn(
                        buf, ImGuiTableColumnFlags_WidthFixed, 22.0f);
                }
                ImGui::TableHeadersRow();

                ImGuiListClipper clipper;
                clipper.Begin((0xFFFF + 1) / 16);
                while (clipper.Step()) {
                    for (int row = clipper.DisplayStart;
                         row < clipper.DisplayEnd; row++) {
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
                                    "%02X",
                                    ImGuiInputTextFlags_CharsHexadecimal)) {
                                mem.Write(addr, val);
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
        ImGui::InputScalar("End", ImGuiDataType_U16, &endAddr, NULL, NULL,
                           "%04X", ImGuiInputTextFlags_CharsHexadecimal);
        ImGui::PopItemWidth();

        if (ImGui::Button("Fill Random Junk")) {
            for (int i = startAddr; i <= (int)endAddr; i++) {
                mem.WriteROM((Word)i, (Byte)(rand() % 256));
            }
        }
    }
    ImGui::EndChild();

    ImGui::End();
}

}  // namespace GUI
