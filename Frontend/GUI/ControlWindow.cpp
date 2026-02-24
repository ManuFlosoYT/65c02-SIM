#include "Frontend/GUI/ControlWindow.h"

#include <ImGuiFileDialog.h>

#include <chrono>
#include <ctime>
#include <filesystem>
#include <iostream>
#include <sstream>

#include "Frontend/Control/Console.h"
#include "Frontend/GUI/Debugger/DebugMenu.h"

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
    if (ImGui::Button("Settings")) {
        ImGui::OpenPopup("SettingsMenu");
    }
    ImGui::SameLine();
    if (ImGui::Button("Debugger")) {
        state.debuggerOpen = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset")) {
        bool wasRunning = !state.emulator.IsPaused();
        if (wasRunning) state.emulator.Pause();

        Console::Clear();
        state.emulator.GetGPU().Init();
        state.emulator.ClearProfiler();

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
    if (ImGui::Checkbox("GPU", &state.gpuEnabled)) {
        state.emulator.SetGPUEnabled(state.gpuEnabled);
    }
    ImGui::SameLine();
    bool soundEnabled = state.emulator.GetSID().IsSoundEnabled();
    if (ImGui::Checkbox("SID", &soundEnabled)) {
        state.emulator.GetSID().EnableSound(soundEnabled);
    }

    if (ImGui::BeginPopup("SettingsMenu")) {
        if (ImGui::Checkbox("Cycle-Accurate", &state.cycleAccurate)) {
            state.emulator.SetCycleAccurate(state.cycleAccurate);
        }
        ImGui::Checkbox("Ignore Save State Hash", &state.ignoreSaveStateHash);
        if (ImGui::Checkbox("Auto-Reload Bin", &state.autoReload)) {
            state.emulator.SetAutoReload(state.autoReload);
        }

        ImGui::Separator();

        if (ImGui::Button("Save State")) {
            // Build default filename: SIM65C02SST_<bin>_<date>.savestate
            std::string binName = "unknown";
            if (!state.bin.empty()) {
                binName = std::filesystem::path(state.bin).stem().string();
            }
            auto now = std::chrono::system_clock::now();
            std::time_t t = std::chrono::system_clock::to_time_t(now);
            std::tm* tm = std::localtime(&t);
            std::ostringstream dateStr;
            dateStr << (tm->tm_year + 1900) << (tm->tm_mon + 1 < 10 ? "0" : "")
                    << (tm->tm_mon + 1) << (tm->tm_mday < 10 ? "0" : "")
                    << tm->tm_mday << "_" << (tm->tm_hour < 10 ? "0" : "")
                    << tm->tm_hour << (tm->tm_min < 10 ? "0" : "") << tm->tm_min
                    << (tm->tm_sec < 10 ? "0" : "") << tm->tm_sec;
            std::string defaultName =
                "SIM65C02SST_" + binName + "_" + dateStr.str();
            ImGuiFileDialog::Instance()->OpenDialog("SaveStateDlgKey",
                                                    "Save State", ".savestate",
                                                    ".", defaultName);
            ImGui::CloseCurrentPopup();
        }
        if (ImGui::Button("Load State")) {
            ImGuiFileDialog::Instance()->OpenDialog(
                "LoadStateDlgKey", "Load State", ".savestate", ".");
            ImGui::CloseCurrentPopup();
        }

        ImGui::Separator();
        ImGui::Text("CRT Filters (GPU)");
        auto setCRTAll = [&](bool v) {
            state.crtScanlines = state.crtInterlacing = state.crtCurvature =
                state.crtChromatic = state.crtBlur = state.crtShadowMask =
                    state.crtVignette = state.crtCornerRounding =
                        state.crtGlassGlare = state.crtColorBleeding =
                            state.crtNoise = state.crtVSyncJitter =
                                state.crtPhosphorDecay = state.crtBloom = v;
        };
        if (ImGui::Button("All On")) setCRTAll(true);
        ImGui::SameLine();
        if (ImGui::Button("All Off")) setCRTAll(false);
        ImGui::Text("Essentials");
        ImGui::Checkbox("Scanlines", &state.crtScanlines);
        ImGui::Checkbox("Interlacing", &state.crtInterlacing);
        ImGui::Checkbox("Screen Curvature", &state.crtCurvature);
        ImGui::Checkbox("Chromatic Aberration", &state.crtChromatic);
        ImGui::Checkbox("Phosphor Blur", &state.crtBlur);
        ImGui::Text("Screen Physicality");
        ImGui::Checkbox("Shadow Mask", &state.crtShadowMask);
        ImGui::Checkbox("Vignette", &state.crtVignette);
        ImGui::Checkbox("Corner Rounding", &state.crtCornerRounding);
        ImGui::Checkbox("Glass Glare", &state.crtGlassGlare);
        ImGui::Text("Signal & Analog");
        ImGui::Checkbox("Color Bleeding", &state.crtColorBleeding);
        ImGui::Checkbox("RF Noise", &state.crtNoise);
        ImGui::Checkbox("VSync Jitter", &state.crtVSyncJitter);
        ImGui::Checkbox("Phosphor Decay", &state.crtPhosphorDecay);
        ImGui::Text("Lighting");
        ImGui::Checkbox("Bloom", &state.crtBloom);

        ImGui::EndPopup();
    }

    DrawDebugMenu(state);

    int tempIPS = state.instructionsPerFrame;
    char targetLabel[32];
    snprintf(targetLabel, sizeof(targetLabel), "Target %s",
             state.cycleAccurate ? "Hz" : "IPS");

    ImGui::SetNextItemWidth(mainColWidth * 0.5f);
    if (ImGui::InputInt(targetLabel, &tempIPS, 100000, 100000)) {
        if (state.instructionsPerFrame == 1 && tempIPS == 100001)
            tempIPS = 100000;

        if (tempIPS < 1) tempIPS = 1;
        state.instructionsPerFrame = tempIPS;
        state.emulator.SetTargetIPS(state.instructionsPerFrame);
    }

    ImGui::SameLine();
    ImGui::Text(" || ");

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
    ImGui::SetScrollHereY(1.0f);
    ImGui::End();
}

}  // namespace GUI
