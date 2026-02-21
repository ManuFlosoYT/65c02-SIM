#include "Frontend/GUI/ControlWindow.h"

#include <ImGuiFileDialog.h>

#include <iostream>

#include "Frontend/Control/Console.h"

using namespace Control;
using namespace Core;
using namespace Hardware;

namespace GUI {

void DrawControlWindow(AppState& state, ImVec2 work_pos, ImVec2 work_size,
                       float top_section_height,
                       ImGuiWindowFlags window_flags) {
    float mainColWidth = work_size.x * 0.47f;

    ImGui::SetNextWindowPos(work_pos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(mainColWidth, top_section_height * 0.4f),
                             ImGuiCond_Always);
    ImGui::Begin("Control", nullptr,
                 window_flags | ImGuiWindowFlags_NoScrollbar |
                     ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::BeginDisabled(!state.romLoaded);
    if (ImGui::Button(state.emulator.IsPaused() ? "Run" : "Pause")) {
        if (state.emulator.IsPaused())
            state.emulator.Resume();
        else
            state.emulator.Pause();
        state.emulator.GetSID().SetEmulationPaused(state.emulator.IsPaused());
    }
    ImGui::SameLine();
    if (ImGui::Button("Step")) {
        state.emulator.GetSID().SetEmulationPaused(false);
        state.emulator.Pause();
        state.emulator.Step();
        state.emulator.GetSID().SetEmulationPaused(true);
    }
    ImGui::EndDisabled();
    ImGui::SameLine();
    if (ImGui::Button("Reset")) {
        bool wasRunning = !state.emulator.IsPaused();
        if (wasRunning) state.emulator.Pause();

        Console::Clear();
        state.emulator.GetGPU().Init();

        if (state.romLoaded) {
            std::string errorMsg;
            if (!state.emulator.Init(state.bin, errorMsg)) {
                std::cerr << "Error resetting ROM: " << errorMsg << std::endl;
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
    ImGui::SameLine();
    if (ImGui::Button(state.gpuEnabled ? "GPU (On)" : "GPU (Off)")) {
        state.gpuEnabled = !state.gpuEnabled;
        state.emulator.SetGPUEnabled(state.gpuEnabled);
    }
    ImGui::SameLine();
    bool soundEnabled = state.emulator.GetSID().IsSoundEnabled();
    if (ImGui::Button(soundEnabled ? "SID (On)" : "SID (Off)")) {
        state.emulator.GetSID().EnableSound(!soundEnabled);
    }
    ImGui::SameLine();
    if (ImGui::Button(state.cycleAccurate ? "Cycle-Accurate (On)"
                                          : "Cycle-Accurate (Off)")) {
        state.cycleAccurate = !state.cycleAccurate;
        state.emulator.SetCycleAccurate(state.cycleAccurate);
    }
    ImGui::SameLine();
    {
        int actualIPS = state.emulator.GetActualIPS();
        if (state.cycleAccurate) {
            if (actualIPS >= 1000000)
                ImGui::Text("Sim: %.2f MHz", actualIPS / 1000000.0f);
            else if (actualIPS >= 1000)
                ImGui::Text("Sim: %.2f KHz", actualIPS / 1000.0f);
            else
                ImGui::Text("Sim: %d Hz", actualIPS);
        } else {
            if (actualIPS >= 1000000)
                ImGui::Text("Sim: %.2f MIPS", actualIPS / 1000000.0f);
            else if (actualIPS >= 1000)
                ImGui::Text("Sim: %.2f KIPS", actualIPS / 1000.0f);
            else
                ImGui::Text("Sim: %d IPS", actualIPS);
        }
    }

    int tempIPS = state.instructionsPerFrame;
    char targetLabel[32];
    snprintf(targetLabel, sizeof(targetLabel), "Target %s",
             state.cycleAccurate ? "Hz" : "IPS");
    if (ImGui::InputInt(targetLabel, &tempIPS, 100000, 100000)) {
        if (state.instructionsPerFrame == 1 && tempIPS == 100001)
            tempIPS = 100000;

        if (tempIPS < 1) tempIPS = 1;
        state.instructionsPerFrame = tempIPS;
        state.emulator.SetTargetIPS(state.instructionsPerFrame);
    }
    ImGui::SetScrollHereY(1.0f);
    ImGui::End();
}

}  // namespace GUI
