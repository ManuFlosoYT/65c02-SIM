#include "Frontend/UI/UIModules.h"
#include <ImGuiFileDialog.h>
#include <imgui.h>
#include "Frontend/Control/AppState.h"
#include "Frontend/Control/Console.h"
#include "Frontend/Control/CartridgeUtils.h"
#include "Hardware/Core/CartridgeLoader.h"
#include <iostream>

using namespace Control;
using namespace Core;

namespace Frontend {

static void HandleROMFilePicker(AppState& state) {
    ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
    if (ImGuiFileDialog::Instance()->Display("ChooseFileDlgKey")) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
            state.emulator.Pause();
            std::string errorMsg;
            if (state.emulator.Init(filePathName, errorMsg)) {
                Console::Clear();
                state.rom.bin = filePathName;
                state.rom.loaded = true;
                state.rom.symbols.Clear();
                state.emulator.SetGPUEnabled(state.emulation.gpuEnabled);
                state.emulator.ClearProfiler();
            } else {
                ImGui::OpenPopup("ErrorLoadingROM");
            }
        }
        ImGuiFileDialog::Instance()->Close();
    }
}

static void HandleCartridgeFilePicker(AppState& state) {
    ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
    if (ImGuiFileDialog::Instance()->Display("ChooseCartridgeDlgKey")) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
            state.emulator.Pause();
            std::string errorMsg;
            Core::Cartridge cart;
            if (Core::CartridgeLoader::Load(filePathName, cart, errorMsg)) {
                Control::ApplyCartridgeConfig(state, cart);
                ImGui::OpenPopup("Cartridge Loaded");
                if (state.emulator.InitFromMemory(cart.romData.data(), cart.romData.size(), cart.romFileName, errorMsg)) {
                    state.rom.bin = filePathName;
                    state.rom.loaded = true;
                    state.rom.symbols.Clear();
                    state.emulator.ClearProfiler();
                } else {
                    ImGui::OpenPopup("ErrorLoadingROM");
                }
            } else {
                std::cerr << "Cartridge load error: " << errorMsg << "\n";
                ImGui::OpenPopup("ErrorLoadingROM");
            }
        }
        ImGuiFileDialog::Instance()->Close();
    }
}

static void HandleSaveStateFilePicker(AppState& state) {
    ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
    if (ImGuiFileDialog::Instance()->Display("SaveStateDlgKey")) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
            state.emulator.Pause();
            if (!state.emulator.SaveState(filePathName)) {
                ImGui::OpenPopup("ErrorSavingState");
            }
        }
        ImGuiFileDialog::Instance()->Close();
    }
}

static void HandleLoadStateFilePicker(AppState& state) {
    ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
    if (ImGuiFileDialog::Instance()->Display("LoadStateDlgKey")) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
            state.emulator.Pause();
            state.emulator.LoadState(filePathName, state.emulation.forceLoadSaveState);
            auto loadResult = state.emulator.GetLastLoadResult();
            bool loadedOk = loadResult == SavestateLoadResult::Success ||
                            loadResult == SavestateLoadResult::VersionMismatch ||
                            loadResult == SavestateLoadResult::HashMismatch;

            if (loadedOk) {
                state.rom.loaded = true;
                state.emulation.gpuEnabled = state.emulator.IsGPUEnabled();
                state.emulation.instructionsPerFrame = state.emulator.GetTargetIPS();
                state.emulation.cycleAccurate = state.emulator.IsCycleAccurate();
                state.emulation.autoReload = state.emulator.IsAutoReloadEnabled();
                state.rom.bin = state.emulator.GetCurrentBinPath();
            }

            if (loadResult != SavestateLoadResult::Success) {
                ImGui::OpenPopup("SavestateFeedback");
            }
        }
        ImGuiFileDialog::Instance()->Close();
    }
}

void HandleDialogs(AppState& state) {
#ifndef TARGET_WASM
    HandleROMFilePicker(state);
    HandleCartridgeFilePicker(state);
    HandleSaveStateFilePicker(state);
    HandleLoadStateFilePicker(state);
#else
    (void)state;
#endif
}

}  // namespace Frontend
