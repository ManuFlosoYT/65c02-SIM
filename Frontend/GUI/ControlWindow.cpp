#include "Frontend/GUI/ControlWindow.h"

#include <ImGuiFileDialog.h>

#include <chrono>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>

#include "Frontend/Control/Console.h"
#include "Frontend/GUI/Debugger/DebugMenu.h"

using namespace Control;
using namespace Core;
using namespace Hardware;

namespace GUI {

static void DrawControlButtonBar(AppState& state) {
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
        if (wasRunning) {
            state.emulator.Pause();
        }

        Console::Clear();
        state.emulator.GetGPU().Reset();
        state.emulator.ClearProfiler();

        if (state.romLoaded) {
            std::string errorMsg;
            if (!state.emulator.Init(state.bin, errorMsg)) {
                std::cerr << "Error resetting ROM: " << errorMsg << '\n';
                state.romLoaded = false;
                ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgKey", "Choose File", ".bin", ".");
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
        if (state.emulator.IsPaused()) {
            state.emulator.Resume();
        } else {
            state.emulator.Pause();
        }
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
}

static void DrawSettingsBasic(AppState& state) {
    if (ImGui::Checkbox("Cycle-Accurate", &state.cycleAccurate)) {
        state.emulator.SetCycleAccurate(state.cycleAccurate);
    }
    ImGui::Checkbox("Force load savestate", &state.forceLoadSaveState);
    if (ImGui::Checkbox("Auto-Reload Bin", &state.autoReload)) {
        state.emulator.SetAutoReload(state.autoReload);
    }
}

static void DrawSettingsSaveState(AppState& state) {
    if (ImGui::Button("Save State")) {
        // Build default filename: SIM65C02SST_<bin>_<date>.savestate
        std::string binName = "unknown";
        if (!state.bin.empty()) {
            binName = std::filesystem::path(state.bin).stem().string();
        }
        auto now = std::chrono::system_clock::now();
        std::time_t timeValue = std::chrono::system_clock::to_time_t(now);
        std::tm* timeStruct = std::localtime(&timeValue);
        std::ostringstream dateStr;
        dateStr << (timeStruct->tm_year + 1900) << (timeStruct->tm_mon + 1 < 10 ? "0" : "") << (timeStruct->tm_mon + 1)
                << (timeStruct->tm_mday < 10 ? "0" : "") << timeStruct->tm_mday << "_"
                << (timeStruct->tm_hour < 10 ? "0" : "") << timeStruct->tm_hour << (timeStruct->tm_min < 10 ? "0" : "")
                << timeStruct->tm_min << (timeStruct->tm_sec < 10 ? "0" : "") << timeStruct->tm_sec;
        std::string defaultName = "SIM65C02SST_" + binName + "_" + dateStr.str();
        ImGuiFileDialog::Instance()->OpenDialog("SaveStateDlgKey", "Save State", ".savestate", ".", defaultName);
        ImGui::CloseCurrentPopup();
    }
    if (ImGui::Button("Load State")) {
        ImGuiFileDialog::Instance()->OpenDialog("LoadStateDlgKey", "Load State", ".savestate", ".");
        ImGui::CloseCurrentPopup();
    }
}

static void DrawSettingsCRT(AppState& state) {
    ImGui::TextUnformatted("CRT Filters (GPU)");
    auto setCRTAll = [&](bool val) {
        state.crtScanlines = state.crtInterlacing = state.crtCurvature = state.crtChromatic = state.crtBlur =
            state.crtShadowMask = state.crtVignette = state.crtCornerRounding = state.crtGlassGlare =
                state.crtColorBleeding = state.crtNoise = state.crtVSyncJitter = state.crtPhosphorDecay =
                    state.crtBloom = val;
    };
    if (ImGui::Button("All On")) {
        setCRTAll(true);
    }
    ImGui::SameLine();
    if (ImGui::Button("All Off")) {
        setCRTAll(false);
    }
    ImGui::TextUnformatted("Essentials");
    ImGui::Checkbox("Scanlines", &state.crtScanlines);
    ImGui::Checkbox("Interlacing", &state.crtInterlacing);
    ImGui::Checkbox("Screen Curvature", &state.crtCurvature);
    ImGui::Checkbox("Chromatic Aberration", &state.crtChromatic);
    ImGui::Checkbox("Phosphor Blur", &state.crtBlur);
    ImGui::TextUnformatted("Screen Physicality");
    ImGui::Checkbox("Shadow Mask", &state.crtShadowMask);
    ImGui::Checkbox("Vignette", &state.crtVignette);
    ImGui::Checkbox("Corner Rounding", &state.crtCornerRounding);
    ImGui::Checkbox("Glass Glare", &state.crtGlassGlare);
    ImGui::TextUnformatted("Signal & Analog");
    ImGui::Checkbox("Color Bleeding", &state.crtColorBleeding);
    ImGui::Checkbox("RF Noise", &state.crtNoise);
    ImGui::Checkbox("VSync Jitter", &state.crtVSyncJitter);
    ImGui::Checkbox("Phosphor Decay", &state.crtPhosphorDecay);
    ImGui::TextUnformatted("Lighting");
    ImGui::Checkbox("Bloom", &state.crtBloom);
}

static void DrawSettingsPopup(AppState& state) {
    if (ImGui::BeginPopup("SettingsMenu")) {
        DrawSettingsBasic(state);
        ImGui::Separator();
        DrawSettingsSaveState(state);
        ImGui::Separator();
        DrawSettingsCRT(state);

        ImGui::EndPopup();
    }
}

static void DrawIPSSection(AppState& state, float mainColWidth) {
    int tempIPS = state.instructionsPerFrame;
    std::string targetLabel = "Target ";
    targetLabel += (state.cycleAccurate ? "Hz" : "IPS");

    ImGui::SetNextItemWidth(mainColWidth * 0.5F);
    if (ImGui::InputInt(targetLabel.c_str(), &tempIPS, 100000, 100000)) {
        if (state.instructionsPerFrame == 1 && tempIPS == 100001) {
            tempIPS = 100000;
        }

        tempIPS = std::max(1, tempIPS);
        state.instructionsPerFrame = tempIPS;
        state.emulator.SetTargetIPS(state.instructionsPerFrame);
    }

    ImGui::SameLine();
    ImGui::TextUnformatted(" || ");

    ImGui::SameLine();
    {
        int actualIPS = state.emulator.GetActualIPS();
        std::ostringstream oss;
        if (state.cycleAccurate) {
            if (actualIPS >= 1000000) {
                oss << "Sim: " << std::fixed << std::setprecision(2) << ((float)actualIPS / 1000000.0F) << " MHz";
            } else if (actualIPS >= 1000) {
                oss << "Sim: " << std::fixed << std::setprecision(2) << ((float)actualIPS / 1000.0F) << " KHz";
            } else {
                oss << "Sim: " << actualIPS << " Hz";
            }
        } else {
            if (actualIPS >= 1000000) {
                oss << "Sim: " << std::fixed << std::setprecision(2) << ((float)actualIPS / 1000000.0F) << " MIPS";
            } else if (actualIPS >= 1000) {
                oss << "Sim: " << std::fixed << std::setprecision(2) << ((float)actualIPS / 1000.0F) << " KIPS";
            } else {
                oss << "Sim: " << actualIPS << " IPS";
            }
        }
        ImGui::TextUnformatted(oss.str().c_str());
    }
}

void DrawControlWindow(AppState& state, ImVec2 work_pos, ImVec2 work_size, float top_section_height,
                       ImGuiWindowFlags window_flags) {
    float mainColWidth = work_size.x * 0.47F;

    ImGui::SetNextWindowPos(work_pos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(mainColWidth, top_section_height * 0.4F), ImGuiCond_Always);
    ImGui::Begin("Control", nullptr, window_flags | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    DrawControlButtonBar(state);
    DrawSettingsPopup(state);
    DrawDebugMenu(state);
    DrawIPSSection(state, mainColWidth);

    ImGui::SetScrollHereY(1.0F);
    ImGui::End();
}

}  // namespace GUI
