#include "Frontend/GUI/Debugger/DebugMenu.h"

#include "Frontend/GUI/CustomFileDialog.h"
#include <imgui.h>

#include <array>

#include "Frontend/Control/CartridgeUtils.h"
#include "Hardware/Core/Console.h"
#include "Frontend/GUI/Debugger/DebuggerWindow.h"
#include "Frontend/GUI/Debugger/DisassemblerWindow.h"
#include "Frontend/GUI/Debugger/MemoryLayoutWindow.h"
#include "Frontend/GUI/Debugger/DataInspectorWindow.h"
#include "Frontend/GUI/Debugger/MemoryScannerWindow.h"
#include "Frontend/GUI/Debugger/ProfilerWindow.h"
#include "Frontend/GUI/Debugger/BreakpointWindow.h"
#include "Frontend/GUI/Debugger/LogicAnalyzerWindow.h"
#include "Frontend/GUI/Debugger/NetworkSnifferWindow.h"
#include "Frontend/GUI/Debugger/SettingsWindow.h"

namespace GUI {

namespace {

void DrawDebugSidebar(Control::AppState& state, float sidebarWidth) {
    constexpr std::array<const char*, 10> modes = {"Settings", "Disassembly", "Profiler", "Debugger", "Breakpoints",
                                                   "Memory Layout", "Data Inspector", "Logic Analyzer", "Network Sniffer",
                                                   "Memory Scanner"};
    ImGui::BeginChild("DebugSidebar", ImVec2(sidebarWidth, 0), ImGuiChildFlags_None, ImGuiWindowFlags_NoScrollbar);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 6));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 2));
    for (size_t i = 0; i < modes.size(); i++) {
        bool selected = (state.debugger.mode == static_cast<Control::DebuggerMode>(i));

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
    }
    ImGui::PopStyleVar(2);
}

void DrawDebugControlButtons(Control::AppState& state, float sidebarWidth) {
    float buttonHeight = ImGui::GetFrameHeightWithSpacing();
    float totalBottomSpace = (buttonHeight * 7.0F);
    float horizontalPadding = 5.0F;
    float adjustedWidth = sidebarWidth - (horizontalPadding * 2.0F);

    ImGui::SetCursorPosY(ImGui::GetWindowHeight() - totalBottomSpace);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 6));

    bool isPaused = state.backend ? state.backend->IsPaused() : true;
    ImGui::BeginDisabled(!state.rom.loaded || !state.backend);
    ImGui::SetCursorPosX(horizontalPadding);
    if (ImGui::Button(isPaused ? "Run " : "Pause", ImVec2(adjustedWidth, 0))) {
        if (state.backend) {
            if (state.backend->IsPaused()) {
                state.backend->Resume();
            } else {
                state.backend->Pause();
            }
            state.backend->GetSID().SetEmulationPaused(state.backend->IsPaused());
        }
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        ImGui::BeginTooltip();
        if (!state.rom.loaded) {
            ImGui::TextUnformatted("Load a ROM to start execution.");
        } else if (isPaused) {
            ImGui::TextUnformatted("Resume CPU execution.");
        } else {
            ImGui::TextUnformatted("Pause CPU execution.");
        }
        ImGui::EndTooltip();
    }

    ImGui::SetCursorPosX(horizontalPadding);
    if (ImGui::Button("Step", ImVec2(adjustedWidth, 0))) {
        if (state.backend) {
            state.backend->DecayHeatmap(48);
            state.backend->GetSID().SetEmulationPaused(false);
            state.backend->Pause();
            state.backend->Step();
            state.backend->GetSID().SetEmulationPaused(true);
        }
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        ImGui::BeginTooltip();
        if (!state.rom.loaded) {
            ImGui::TextUnformatted("Load a ROM to step.");
        } else {
            ImGui::TextUnformatted("Execute a single CPU instruction.");
        }
        ImGui::EndTooltip();
    }

    bool canStepBack = state.backend && state.backend->CanRewind() && state.backend->IsPaused();
    ImGui::BeginDisabled(!canStepBack);
    ImGui::SetCursorPosX(horizontalPadding);
    if (ImGui::Button("Step Back", ImVec2(adjustedWidth, 0))) {
        if (state.backend) {
            state.backend->GetSID().SetEmulationPaused(false);
            state.backend->Pause();
            state.backend->StepBack();
            state.backend->GetSID().SetEmulationPaused(true);
        }
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        ImGui::BeginTooltip();
        if (!canStepBack) {
            ImGui::TextUnformatted("Emulator must be paused and have history to step back.");
        } else {
            ImGui::TextUnformatted("Step back to previous CPU instruction.");
        }
        ImGui::EndTooltip();
    }
    ImGui::EndDisabled();
    ImGui::EndDisabled();

    ImGui::SetCursorPosX(horizontalPadding);
    if (ImGui::Button("Reset", ImVec2(adjustedWidth, 0))) {
        Control::PromptOrExecuteCartridgeAction(state, Control::PendingCartridgeAction::Reset);
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        ImGui::BeginTooltip();
        ImGui::TextUnformatted("Reset CPU and system state.");
        ImGui::EndTooltip();
    }

    ImGui::Spacing();

    ImGui::SetCursorPosX(horizontalPadding);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered));
    if (ImGui::Button("Exit", ImVec2(adjustedWidth, 0))) {
        state.debugger.open = false;
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        ImGui::BeginTooltip();
        ImGui::TextUnformatted("Close the debug menu.");
        ImGui::EndTooltip();
    }
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar();
    ImGui::EndChild();
}

}  // namespace

void DrawDebugMenu(Control::AppState& state) {
    if (!state.debugger.open) {
        if (state.backend) {
            state.backend->SetProfilingEnabled(false);
            state.backend->SetHeatmapEnabled(false);
            state.backend->SetLogicAnalyzerEnabled(false);
        }
        return;
    }

    ImVec2 vpSize = ImGui::GetMainViewport()->Size;
    ImGui::SetNextWindowSize(ImVec2(vpSize.x * 0.75F, vpSize.y * 0.75F), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Debug Menu", &state.debugger.open, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize)) {
        if (state.backend) {
            state.backend->SetProfilingEnabled(false);
            state.backend->SetHeatmapEnabled(false);
            state.backend->SetLogicAnalyzerEnabled(false);
        }
        ImGui::End();
        return;
    }

    const float sidebarWidth = 165.0F * state.render.uiScale;
    DrawDebugSidebar(state, sidebarWidth);
    DrawDebugControlButtons(state, sidebarWidth);

    ImGui::SameLine();

    float lineX = ImGui::GetCursorScreenPos().x - ImGui::GetStyle().ItemSpacing.x;
    ImGui::GetWindowDrawList()->AddLine(ImVec2(lineX, ImGui::GetWindowPos().y),
                                        ImVec2(lineX, ImGui::GetWindowPos().y + ImGui::GetWindowHeight()),
                                        ImGui::GetColorU32(ImGuiCol_Separator), 1.0F);

    ImGui::BeginChild("DebugContent", ImVec2(0, 0), ImGuiChildFlags_None);

    if (state.backend) {
        state.backend->SetProfilingEnabled(state.debugger.open && state.debugger.mode == Control::DebuggerMode::Profiler);
        state.backend->SetHeatmapEnabled(state.debugger.open && state.debugger.mode == Control::DebuggerMode::Debugger &&
                                         state.debugger.heatmapEnabled);
        state.backend->SetLogicAnalyzerEnabled(state.debugger.open && state.debugger.mode == Control::DebuggerMode::LogicAnalyzer);
    }

    if (state.debugger.mode == Control::DebuggerMode::Settings) {
        DrawSettingsContent(state);
    } else if (state.debugger.mode == Control::DebuggerMode::Disassembly) {
        DrawDisassemblerContent(state);
    } else if (state.debugger.mode == Control::DebuggerMode::Profiler) {
        DrawProfilerWindow(state);
    } else if (state.debugger.mode == Control::DebuggerMode::Debugger) {
        DrawDebuggerWindow(state);
    } else if (state.debugger.mode == Control::DebuggerMode::Breakpoints) {
        DrawBreakpointWindow(state);
    } else if (state.debugger.mode == Control::DebuggerMode::MemoryLayout) {
        DrawMemoryLayoutWindow(state);
    } else if (state.debugger.mode == Control::DebuggerMode::DataInspector) {
        DrawDataInspectorWindow(state);
    } else if (state.debugger.mode == Control::DebuggerMode::LogicAnalyzer) {
        DrawLogicAnalyzerWindow(state);
    } else if (state.debugger.mode == Control::DebuggerMode::MemoryScanner) {
        DrawMemoryScannerWindow(state);
    } else {
        DrawNetworkSnifferContent(state);
    }
    ImGui::EndChild();

    ImGui::End();
}

}  // namespace GUI
