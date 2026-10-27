#include "Frontend/GUI/FileDialogs.h"
#include "Frontend/GUI/CustomFileDialog.h"
#include "Frontend/Control/AppState.h"
#include "Hardware/Core/Console.h"
#include "Frontend/Control/CartridgeUtils.h"
#include "Hardware/Core/CartridgeLoader.h"
#include "Frontend/Control/SDUtils.h"
#ifdef TARGET_WASM
#include "Frontend/web/WebFileUtils.h"
#endif

#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>

using namespace Control;
using namespace Core;

namespace Frontend {

static void HandleROMFilePicker(AppState& state) {
    if (CustomFileDialog::IsOpened("ChooseFileDlgKey")) {
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        const ImVec2 targetSize = ImVec2(viewport->WorkSize.x * 0.75F, viewport->WorkSize.y * 0.75F);
        ImGui::SetNextWindowPos(viewport->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5F, 0.5F));
        ImGui::SetNextWindowSize(targetSize, ImGuiCond_Appearing);
    }
    if (CustomFileDialog::Display("ChooseFileDlgKey")) {
        if (CustomFileDialog::IsOk()) {
            std::string filePathName = CustomFileDialog::GetFilePathName();
            state.emulator.Pause();
            std::string errorMsg;
            if (filePathName.ends_with(".65c") || filePathName.ends_with(".c65")) {
                Core::Cartridge cart;
                if (Core::CartridgeLoader::Load(filePathName, cart, errorMsg)) {
                    Control::ApplyCartridgeConfig(state, cart);
                    ImGui::OpenPopup("Cartridge Loaded");
                    if (state.emulator.InitFromMemory(cart.romData, cart.romFileName, errorMsg)) {
                        state.rom.bin = filePathName;
                        state.rom.loaded = true;
                        state.emulator.ClearProfiler();
                    } else {
                        ImGui::OpenPopup("ErrorLoadingROM");
                    }
                } else {
                    std::cerr << "Cartridge load error: " << errorMsg << "\n";
                    ImGui::OpenPopup("ErrorLoadingROM");
                }
            } else {
                if (state.emulator.Init(filePathName, errorMsg)) {
                    Console::Clear();
                    state.rom.bin = filePathName;
                    state.rom.loaded = true;
                    state.rom.symbols.Clear();
                    state.emulator.ClearProfiler();
                    state.emulator.ClearCartridge();
                } else {
                    ImGui::OpenPopup("ErrorLoadingROM");
                }
            }
        }
        CustomFileDialog::Close();
    }
}

static void HandleSaveStateFilePicker(AppState& state) {
    if (CustomFileDialog::Display("SaveStateDlgKey")) {
        if (CustomFileDialog::IsOk()) {
            std::string filePathName = CustomFileDialog::GetFilePathName();
            state.emulator.Pause();
            if (!state.emulator.SaveState(filePathName)) {
                ImGui::OpenPopup("ErrorSavingState");
            }
        }
        CustomFileDialog::Close();
    }
}

static void HandleLoadStateFilePicker(AppState& state) {
    if (CustomFileDialog::Display("LoadStateDlgKey")) {
        if (CustomFileDialog::IsOk()) {
            std::string filePathName = CustomFileDialog::GetFilePathName();
            state.emulator.Pause();
            state.emulator.LoadState(filePathName, state.emulation.forceLoadSaveState);
            auto loadResult = state.emulator.GetLastLoadResult();
            bool loadedOk = loadResult == SavestateLoadResult::Success ||
                            loadResult == SavestateLoadResult::VersionMismatch ||
                            loadResult == SavestateLoadResult::HashMismatch;

            if (loadedOk) {
                Control::SyncUIStateFromEmulator(state);
            }

            if (loadResult != SavestateLoadResult::Success) {
                ImGui::OpenPopup("SavestateFeedback");
            }
        }
        CustomFileDialog::Close();
    }
}

void HandleCreateSDDialog(AppState& state) {
    if (CustomFileDialog::Display("CreateSDDlgKey")) {
        if (CustomFileDialog::IsOk()) {
            std::string filePath = CustomFileDialog::GetFilePathName();
            // Ensure .img extension
            if (filePath.size() < 4 || filePath.substr(filePath.size() - 4) != ".img") {
                filePath += ".img";
            }
            std::string tempPath = filePath;
#ifdef TARGET_WASM
            tempPath = "temp.img";
#endif
            if (GUI::CreateFAT16Image(tempPath)) {
#ifdef TARGET_WASM
                std::ifstream ifs(tempPath, std::ios::binary);
                std::vector<uint8_t> buffer((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
                WebFileUtils::download_file("new_sd.img", buffer.data(), buffer.size());
#endif
                if (GUI::IsSDCardEnabled(state)) {
                    state.emulator.GetSDCard().Mount(filePath);
                } else {
                    state.popups.sdCardDisabled = true;
                }
            }
        }
        CustomFileDialog::Close();
    }
}

void HandleMountSDDialog(AppState& state) {
    if (CustomFileDialog::Display("MountSDDlgKey")) {
        if (CustomFileDialog::IsOk()) {
            std::string filePath = CustomFileDialog::GetFilePathName();
            if (GUI::IsSDCardEnabled(state)) {
                state.emulator.GetSDCard().Mount(filePath);
            } else {
                state.popups.sdCardDisabled = true;
            }
        }
        CustomFileDialog::Close();
    }
}

void HandleMountFolderDialog(AppState& state) {
    if (CustomFileDialog::Display("MountFolderDlgKey")) {
        if (CustomFileDialog::IsOk()) {
            std::string folderPath = CustomFileDialog::GetCurrentPath();
            const std::filesystem::path hostFolder(std::u8string(folderPath.begin(), folderPath.end()));
            if (!folderPath.empty() && std::filesystem::is_directory(hostFolder)) {
                if (!GUI::IsSDCardEnabled(state)) {
                    state.popups.sdCardDisabled = true;
                } else {
                    if (std::filesystem::exists(hostFolder / ".65C02_SIM_mount")) {
                        GUI::MountFolder(state, folderPath, true);
                    } else {
                        state.popups.sdCardMountSentinelWarning = true;
                        state.popups.sdCardPendingMountFolder = folderPath;
                    }
                }
            }
        }
        CustomFileDialog::Close();
    }
}

void HandleLoadScriptDialog(AppState& state) {
    if (CustomFileDialog::Display("LoadScriptDlgKey")) {
        if (CustomFileDialog::IsOk()) {
            std::string filePath = CustomFileDialog::GetFilePathName();
            state.script.path = filePath;
            state.script.loaded = true;
            state.script.showConsole = true;
            state.emulator.Pause();  // Pause the frontend before running the script
            state.emulator.GetScriptEngine().LoadAndRun(filePath);
            ImGui::CloseCurrentPopup();
        }
        CustomFileDialog::Close();
    }
}

void HandleDialogs(AppState& state) {
#ifndef TARGET_WASM
    HandleROMFilePicker(state);
    HandleSaveStateFilePicker(state);
    HandleLoadStateFilePicker(state);
    HandleCreateSDDialog(state);
    HandleMountSDDialog(state);
    HandleMountFolderDialog(state);
    HandleLoadScriptDialog(state);
#else
    (void)state;
#endif
}

}  // namespace Frontend
