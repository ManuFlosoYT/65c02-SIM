#include "Frontend/GUI/PopupManager.h"
#include <SDL3/SDL.h>
#include <filesystem>
#include <format>
#include <fstream>
#include <imgui.h>
#include "Frontend/Control/AppState.h"
#include "Frontend/Control/CartridgeUtils.h"
#include "Frontend/GUI/ConsoleWindow.h"
#include "Frontend/GUI/ControlWindow.h"
#include "Frontend/GUI/LCDWindow.h"
#include "Frontend/GUI/ScriptConsoleWindow.h"
#include "Frontend/GUI/SIDViewerWindow.h"
#include "Frontend/Control/SDUtils.h"
#include "Frontend/GUI/UpdatePopup.h"
#include "Frontend/GUI/SDKUpdatePopup.h"
#include "Frontend/GUI/Video/VRAMViewerWindow.h"
#include "Frontend/GUI/IDEWindow.h"
#include "Frontend/GUI/MIDI/MIDIWorkspaceWindow.h"
#include <iostream>
#include "Hardware/Core/CartridgeLoader.h"
#include "Hardware/Core/Console.h"
#ifndef TARGET_WASM
#include "Frontend/GUI/CustomFileDialog.h"
#endif

using namespace Control;
using namespace Core;

namespace Frontend {

static void DrawROMAndStatePopups(AppState& state) {
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();

    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5F, 0.5F));
    if (ImGui::BeginPopupModal("ErrorLoadingROM", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
        ImGui::TextUnformatted("Error loading ROM. Please check the file.");
        if (ImGui::Button("OK", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5F, 0.5F));
    if (ImGui::BeginPopupModal("ErrorSavingState", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
        ImGui::TextUnformatted("Error saving state. Please check your permissions.");
        if (ImGui::Button("OK", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5F, 0.5F));
    if (ImGui::BeginPopupModal("SavestateFeedback", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
        auto result = state.emulator.GetLastLoadResult();
        if (result == SavestateLoadResult::VersionMismatch || result == SavestateLoadResult::HashMismatch) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0F, 1.0F, 0.0F, 1.0F));
            ImGui::TextUnformatted("Warning: Savestate compatibility issue");
            ImGui::PopStyleColor();
            if (result == SavestateLoadResult::VersionMismatch) {
                ImGui::TextUnformatted("Version mismatch detected:");
                ImGui::Bullet();
                ImGui::TextUnformatted(std::format("Saved: {}", state.emulator.GetLastLoadVersion()).c_str());
                ImGui::Bullet();
                ImGui::TextUnformatted(std::format("Current: {}", PROJECT_VERSION).c_str());
            } else {
                ImGui::TextUnformatted(
                    "Hash mismatch. The data might be modified or "
                    "corrupt.");
            }
            ImGui::TextUnformatted(
                "The state was loaded, but some things might not work "
                "correctly.");
        } else if (result == SavestateLoadResult::StructuralError) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0F, 0.0F, 0.0F, 1.0F));
            ImGui::TextUnformatted("Error: Failed to load state");
            ImGui::PopStyleColor();
            ImGui::TextUnformatted("The data is structurally incompatible or corrupted.");
        } else {
            ImGui::TextUnformatted("An unknown error occurred while loading the state.");
        }

        ImGui::Spacing();
        ImGui::SetNextWindowSize(ImVec2(120.0F * state.render.uiScale, 0));
        if (ImGui::Button("OK", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

static void DrawSDCardPopups(AppState& state) {
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();

    if (state.popups.sdCardDisabled) {
        ImGui::OpenPopup("SD Card Disabled");
        state.popups.sdCardDisabled = false;
    }

    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5F, 0.5F));
    if (ImGui::BeginPopupModal("SD Card Disabled", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
        ImGui::TextUnformatted("The SD Card device is currently disabled in the Memory Layout.");
        ImGui::TextUnformatted("Please enable it in the Memory Layout (Debugger -> Memory Layout) to use it.");
        ImGui::Spacing();
        if (ImGui::Button("OK", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (state.popups.sdCardWebWarning) {
        ImGui::OpenPopup("SD Card Web Warning");
        state.popups.sdCardWebWarning = false;
    }

    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5F, 0.5F));
    if (ImGui::BeginPopupModal("SD Card Web Warning", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
        ImGui::TextUnformatted("SD Card Mounted (Web Port)");
        ImGui::Separator();
        ImGui::TextUnformatted("Due to browser security limitations, changes made to the SD card");
        ImGui::TextUnformatted("are stored in a virtual filesystem and NOT synced to your local file.");
        ImGui::Spacing();
        ImGui::TextUnformatted("To save your changes, you MUST use 'Unmount & Save (Download)'");
        ImGui::TextUnformatted("or the 'Save Changes' button in the Control Window.");
        ImGui::Spacing();
        if (ImGui::Button("I Understand", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (state.popups.sdCardMountSentinelWarning) {
        ImGui::OpenPopup("SD Folder Deletion Warning");
        state.popups.sdCardMountSentinelWarning = false;
    }

    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5F, 0.5F));
    if (ImGui::BeginPopupModal("SD Folder Deletion Warning", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
        ImGui::TextUnformatted("The sentinel file '.65C02_SIM_mount' was not found in:");
        ImGui::TextUnformatted(state.popups.sdCardPendingMountFolder.c_str());
        ImGui::Spacing();
        ImGui::TextUnformatted("Without this file, file deletions on the host will be restricted to prevent accidental data loss.");
        ImGui::Spacing();
        if (ImGui::Button("Create .65C02_SIM_mount & Allow Deletion")) {
            const auto& pending = state.popups.sdCardPendingMountFolder;
            std::filesystem::path sentinelPath = std::filesystem::path(std::u8string(pending.begin(), pending.end())) / ".65C02_SIM_mount";
            std::ofstream sentinel(sentinelPath);
            GUI::MountFolder(state, state.popups.sdCardPendingMountFolder, true);
            state.popups.sdCardPendingMountFolder.clear();
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Mount with Deletions Restricted")) {
            GUI::MountFolder(state, state.popups.sdCardPendingMountFolder, false);
            state.popups.sdCardPendingMountFolder.clear();
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            state.popups.sdCardPendingMountFolder.clear();
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

static void DrawCartridgePopups(AppState& state) {
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5F, 0.5F));

    if (ImGui::BeginPopupModal("Cartridge Loaded", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
        const auto& cart = state.emulator.GetCartridge();
        if (cart.loaded) {
            std::string nameLine = "Name: " + cart.metadata.name;
            std::string authorLine = "Author: " + cart.metadata.author;
            std::string versionLine = "Version: " + cart.metadata.version;
            ImGui::TextUnformatted(nameLine.c_str());
            ImGui::TextUnformatted(authorLine.c_str());
            ImGui::TextUnformatted(versionLine.c_str());
            ImGui::Separator();
            ImGui::TextUnformatted(cart.metadata.description.c_str());
        }
        ImGui::Separator();
        if (ImGui::Button("OK", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (state.popups.showCartridgeSavePrompt) {
        ImGui::OpenPopup("Save SD Card Changes?###CartridgeSDSavePrompt");
        state.popups.showCartridgeSavePrompt = false;
    }

    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5F, 0.5F));
    if (ImGui::BeginPopupModal("Save SD Card Changes?###CartridgeSDSavePrompt", nullptr,
                               ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
        ImGui::TextUnformatted("The SD card data in the cartridge has been modified.");
        ImGui::TextUnformatted("Do you want to save changes back to the cartridge (.65c)?");
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        float availWidth = ImGui::GetContentRegionAvail().x;
        float spacing = ImGui::GetStyle().ItemSpacing.x;
        float btnWidth = (availWidth - spacing * 2.0F) / 3.0F;

        if (ImGui::Button("Save", ImVec2(btnWidth, 0))) {
            Frontend::ConfirmCartridgeAction(state, true);
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Don't Save", ImVec2(btnWidth, 0))) {
            Frontend::ConfirmCartridgeAction(state, false);
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(btnWidth, 0))) {
            state.popups.pendingCartridgeAction = Control::PendingCartridgeAction::None;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void DrawPopups(AppState& state) {
    DrawROMAndStatePopups(state);
    DrawSDCardPopups(state);
    DrawCartridgePopups(state);
}

void DrawGUIWindows(AppState& state, const ImVec2& work_pos, const ImVec2& work_size, float top_section_height, unsigned int windowFlags) {
    GUI::DrawUpdatePopup(state);
#ifndef TARGET_WASM
    GUI::DrawSDKUpdatePopup(state);
#endif
    if (state.ide.open && state.ide.mode == Control::IDEMode::MIDI) {
        GUI::DrawSIDViewerWindow(state, work_pos, work_size, top_section_height, static_cast<ImGuiWindowFlags>(windowFlags));
        GUI::DrawMIDIWorkspaceWindow(state, work_pos, work_size, top_section_height, static_cast<ImGuiWindowFlags>(windowFlags));
        return;
    }
    GUI::DrawControlWindow(state, work_pos, work_size, top_section_height, (ImGuiWindowFlags)windowFlags);
    GUI::DrawLCDWindow(state, work_pos, work_size, top_section_height, (ImGuiWindowFlags)windowFlags);
    GUI::DrawSIDViewerWindow(state, work_pos, work_size, top_section_height, (ImGuiWindowFlags)windowFlags);
    GUI::DrawConsoleWindow(state, work_pos, work_size, top_section_height, (ImGuiWindowFlags)windowFlags);
    GUI::DrawScriptConsoleWindow(state, work_pos, work_size, top_section_height, (ImGuiWindowFlags)windowFlags);
    state.crt.time = static_cast<float>(SDL_GetTicks()) / 1000.0F;
    GUI::DrawVRAMViewerWindow(state, work_pos, work_size, top_section_height, (ImGuiWindowFlags)windowFlags);
    GUI::DrawIDEWindow(state);
}

bool ShouldPromptCartridgeSDSave(const Control::AppState& state) {
    const auto& cart = state.emulator.GetCartridge();
    return cart.loaded && !cart.sdCardPath.empty() && state.emulator.GetSDCard().IsMounted() &&
           state.emulator.GetSDCard().IsModified();
}

void ExecuteReset(Control::AppState& state) {
    bool wasRunning = !state.emulator.IsPaused();
    if (wasRunning) {
        state.emulator.Pause();
    }

    Console::Clear();
    state.emulator.GetGPU().Reset();
    state.emulator.ClearProfiler();

    if (state.emulator.GetCartridge().loaded) {
        std::string path = state.emulator.GetCartridge().sourceZipPath;
#ifndef TARGET_WASM
        if (!path.empty()) {
            std::string errorMsg;
            Core::Cartridge cart;
            if (Core::CartridgeLoader::Load(path, cart, errorMsg)) {
                Control::ApplyCartridgeConfig(state, cart);
                state.emulator.InitFromMemory(cart.romData, cart.romFileName, errorMsg);
            } else {
                std::cerr << "Failed to reload cartridge from " << path << ": " << errorMsg << "\n";
                state.emulator.SetupHardware();
            }
        } else {
            state.emulator.SetupHardware();
        }
#else
        state.emulator.SetupHardware();
#endif
    } else if (state.rom.loaded) {
        std::string errorMsg;
#ifdef TARGET_WASM
        if (!state.emulator.InitFromMemory(state.rom.data, state.rom.bin, errorMsg)) {
            state.rom.loaded = false;
        }
#else
        bool initSuccess = false;
        if (!state.rom.data.empty()) {
            initSuccess = state.emulator.InitFromMemory(state.rom.data, state.rom.bin, errorMsg);
        } else {
            initSuccess = state.emulator.Init(state.rom.bin, errorMsg);
        }
        if (!initSuccess) {
            state.rom.loaded = false;
            Frontend::CustomFileDialog::OpenDialog("ChooseFileDlgKey", "Choose File", "Supported Files (*.bin, *.65c){.bin,.65c,.c65},.bin,.65c,.*", ".");
        }
#endif
    }

    if (wasRunning) {
        state.emulator.Resume();
        state.emulator.GetSID().SetEmulationPaused(false);
    }
}

void ExecuteEjectCartridge(Control::AppState& state) {
    state.emulator.GetSDCard().Unmount();
    state.emulator.ClearCartridge();
    state.emulation.gpuEnabled = false;
    state.emulation.espEnabled = false;
    state.emulation.sdEnabled = false;
    state.emulation.cycleAccurate = true;
    state.emulation.instructionsPerFrame = 1000000;
    state.emulator.SetGPUEnabled(state.emulation.gpuEnabled);
    state.emulator.SetESPEnabled(state.emulation.espEnabled);
    state.emulator.SetSDEnabled(state.emulation.sdEnabled);
    state.emulator.SetCycleAccurate(state.emulation.cycleAccurate);
    state.emulator.SetTargetIPS(state.emulation.instructionsPerFrame);
    state.rom.symbols.Clear();
    state.rom.loaded = false;
    state.rom.bin = "";
    state.emulator.Reset();
    state.emulator.Pause();
}

void PromptOrExecuteCartridgeAction(Control::AppState& state, Control::PendingCartridgeAction action) {
    if (Frontend::ShouldPromptCartridgeSDSave(state)) {
        state.emulator.Pause();
        state.popups.showCartridgeSavePrompt = true;
        state.popups.pendingCartridgeAction = action;
        return;
    }

    switch (action) {
        case Control::PendingCartridgeAction::Reset:
            Frontend::ExecuteReset(state);
            break;
        case Control::PendingCartridgeAction::Eject:
            Frontend::ExecuteEjectCartridge(state);
            break;
        case Control::PendingCartridgeAction::Exit:
            state.requestExit = true;
            break;
        case Control::PendingCartridgeAction::None:
            break;
    }
}

void ConfirmCartridgeAction(Control::AppState& state, bool saveChanges) {
    if (saveChanges) {
        const auto& cart = state.emulator.GetCartridge();
        if (cart.loaded && !cart.sdCardPath.empty()) {
            Core::CartridgeLoader::SaveSDToZip(cart);
        }
    }
    state.emulator.GetSDCard().SetModified(false);

    auto action = state.popups.pendingCartridgeAction;
    state.popups.pendingCartridgeAction = Control::PendingCartridgeAction::None;

    switch (action) {
        case Control::PendingCartridgeAction::Reset:
            Frontend::ExecuteReset(state);
            break;
        case Control::PendingCartridgeAction::Eject:
            Frontend::ExecuteEjectCartridge(state);
            break;
        case Control::PendingCartridgeAction::Exit:
            state.requestExit = true;
            break;
        case Control::PendingCartridgeAction::None:
            break;
    }
}

}  // namespace Frontend
