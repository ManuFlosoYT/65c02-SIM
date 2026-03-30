#include "Frontend/GUI/Debugger/DebugMenu.h"

#include <ImGuiFileDialog.h>
#include <imgui.h>

#include <array>

#include "Frontend/Control/Console.h"
#include "Frontend/GUI/Debugger/DebuggerWindow.h"
#include "Frontend/GUI/Debugger/DisassemblerWindow.h"
#include "Frontend/GUI/Debugger/MemoryLayoutWindow.h"
#include "Frontend/GUI/Debugger/ProfilerWindow.h"
#include "Frontend/GUI/Debugger/SettingsWindow.h"

namespace GUI {

namespace {

void DrawDebugSidebar(Control::AppState& state, float sidebarWidth) {
    constexpr std::array<const char*, 5> modes = {"Settings", "Disassembly", "Profiler", "Debugger", "Memory Layout"};
    ImGui::BeginChild("DebugSidebar", ImVec2(sidebarWidth, 0), ImGuiChildFlags_None, ImGuiWindowFlags_NoScrollbar);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 6));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 2));
    for (size_t i = 0; i < modes.size(); i++) {
        bool selected = (state.debugger.mode == static_cast<Control::DebuggerMode>(i));
        bool isDisabled = (i == 4 && state.emulator.GetCartridge().loaded); // Disable Memory Layout for cartridges

        if (isDisabled) {
            ImGui::BeginDisabled();
        }

        if (selected) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
        } else {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        }
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered));
        if (ImGui::Button(modes.at(i), ImVec2(sidebarWidth, 0))) {
            state.debugger.mode = static_cast<Control::DebuggerMode>(i);
        }
        ImGui::PopStyleColor(2);

        if (isDisabled) {
            ImGui::EndDisabled();
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
                if (ImGui::BeginTooltip()) {
                    ImGui::TextUnformatted("Memory Layout is not available for custom cartridges");
                    ImGui::EndTooltip();
                }
            }
        }
    }
    ImGui::PopStyleVar(2);
}

void DrawDebugControlButtons(Control::AppState& state, float sidebarWidth) {
    float buttonHeight = ImGui::GetFrameHeightWithSpacing();
    float totalBottomSpace = (buttonHeight * 6.0F);
    float horizontalPadding = 5.0F;
    float adjustedWidth = sidebarWidth - (horizontalPadding * 2.0F);

    ImGui::SetCursorPosY(ImGui::GetWindowHeight() - totalBottomSpace);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 6));

    // Run/Pause
    ImGui::BeginDisabled(!state.rom.loaded);
    ImGui::SetCursorPosX(horizontalPadding);
    if (ImGui::Button(state.emulator.IsPaused() ? "Run " : "Pause", ImVec2(adjustedWidth, 0))) {
        if (state.emulator.IsPaused()) {
            state.emulator.Resume();
        } else {
            state.emulator.Pause();
        }
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
        if (wasRunning) {
            state.emulator.Pause();
        }
        Console::Clear();
        state.emulator.GetGPU().Reset();
        state.emulator.ClearProfiler();
        if (state.emulator.GetCartridge().loaded) {
            state.emulator.SetupHardware();
        } else if (state.rom.loaded) {
            std::string errorMsg;
            bool initSuccess = false;
            if (!state.rom.data.empty()) {
                initSuccess = state.emulator.InitFromMemory(state.rom.data.data(), state.rom.data.size(), state.rom.bin, errorMsg);
            } else {
                initSuccess = state.emulator.Init(state.rom.bin, errorMsg);
            }
            
            if (!initSuccess) {
                state.rom.loaded = false;
                ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgKey", "Choose File", ".bin", ".");
            } else {
                state.emulator.SetGPUEnabled(state.emulation.gpuEnabled);
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
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered));
    if (ImGui::Button("Exit", ImVec2(adjustedWidth, 0))) {
        state.debugger.open = false;
    }
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar();
    ImGui::EndChild();
}

}  // namespace

void DrawDebugMenu(Control::AppState& state) {
    if (!state.debugger.open) {
        state.emulator.SetProfilingEnabled(false);
        return;
    }

    state.emulator.SetProfilingEnabled(true);

    ImVec2 vpSize = ImGui::GetMainViewport()->Size;
    ImGui::SetNextWindowSize(ImVec2(vpSize.x * 0.75F, vpSize.y * 0.75F), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Debug Menu", &state.debugger.open, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize)) {
        state.emulator.SetProfilingEnabled(false);
        ImGui::End();
        return;
    }

    // Left sidebar
    const float sidebarWidth = 140.0F;
    DrawDebugSidebar(state, sidebarWidth);
    DrawDebugControlButtons(state, sidebarWidth);

    ImGui::SameLine();

    // Vertical separator
    float lineX = ImGui::GetCursorScreenPos().x - ImGui::GetStyle().ItemSpacing.x;
    ImGui::GetWindowDrawList()->AddLine(ImVec2(lineX, ImGui::GetWindowPos().y),
                                        ImVec2(lineX, ImGui::GetWindowPos().y + ImGui::GetWindowHeight()),
                                        ImGui::GetColorU32(ImGuiCol_Separator), 1.0F);

    // Content area
    ImGui::BeginChild("DebugContent", ImVec2(0, 0), ImGuiChildFlags_None);

    if (state.debugger.mode == Control::DebuggerMode::Settings) {
        DrawSettingsContent(state);
    } else if (state.debugger.mode == Control::DebuggerMode::Disassembly) {
        DrawDisassemblerContent(state);
    } else if (state.debugger.mode == Control::DebuggerMode::Profiler) {
        DrawProfilerWindow(state);
    } else if (state.debugger.mode == Control::DebuggerMode::Debugger) {
        DrawDebuggerWindow(state);
    } else {  // Memory Layout
        DrawMemoryLayoutWindow(state);
    }
    ImGui::EndChild();

    ImGui::End();
}

}  // namespace GUI
