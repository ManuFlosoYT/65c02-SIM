#include "Frontend/GUI/PopupManager.h"
#include <filesystem>
#include <format>
#include <memory>
#include <imgui.h>
#include "Frontend/Control/AppState.h"
#include "Frontend/Control/CartridgeUtils.h"
#include "Frontend/Control/SDUtils.h"
#include "Hardware/Core/Logger.h"
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

    ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5F, 0.5F));
    if (ImGui::BeginPopupModal("ErrorLoadingROM", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
        ImGui::TextUnformatted("Error loading ROM. Please check the file.");
        if (ImGui::Button("OK", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5F, 0.5F));
    if (ImGui::BeginPopupModal("ErrorSavingState", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
        ImGui::TextUnformatted("Error saving state. Please check your permissions.");
        if (ImGui::Button("OK", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5F, 0.5F));
    if (ImGui::BeginPopupModal("SavestateFeedback", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
        auto result = state.backend ? state.backend->GetLastLoadResult() : SavestateLoadResult::GenericError;
        if (result == SavestateLoadResult::VersionMismatch || result == SavestateLoadResult::HashMismatch) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0F, 1.0F, 0.0F, 1.0F));
            ImGui::TextUnformatted("Warning: Savestate compatibility issue");
            ImGui::PopStyleColor();
            if (result == SavestateLoadResult::VersionMismatch) {
                ImGui::TextUnformatted("Version mismatch detected:");
                ImGui::Bullet();
                std::string savedVer = state.backend ? state.backend->GetLastLoadVersion() : "unknown";
                ImGui::TextUnformatted(std::format("Saved: {}", savedVer).c_str());
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

    if (state.popups.sdCardDisabled && (!ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId) || ImGui::IsPopupOpen("SD Card Disabled"))) {
        ImGui::OpenPopup("SD Card Disabled");
        state.popups.sdCardDisabled = false;
    }

    ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5F, 0.5F));
    if (ImGui::BeginPopupModal("SD Card Disabled", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
        ImGui::TextUnformatted("The SD Card device is currently disabled in the Memory Layout.");
        ImGui::TextUnformatted("Please enable it in the Memory Layout (Debugger -> Memory Layout) to use it.");
        ImGui::Spacing();
        if (ImGui::Button("OK", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (state.popups.sdCardWebWarning && (!ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId) || ImGui::IsPopupOpen("SD Card Web Warning"))) {
        ImGui::OpenPopup("SD Card Web Warning");
        state.popups.sdCardWebWarning = false;
    }

    ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5F, 0.5F));
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

    if (state.popups.sdCardMountSentinelWarning && (!ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId) || ImGui::IsPopupOpen("SD Folder Deletion Warning"))) {
        ImGui::OpenPopup("SD Folder Deletion Warning");
        state.popups.sdCardMountSentinelWarning = false;
    }

    ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5F, 0.5F));
    if (ImGui::BeginPopupModal("SD Folder Deletion Warning", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
        ImGui::TextUnformatted("The sentinel file '.65C02_SIM_mount' was not found in:");
        ImGui::TextUnformatted(state.popups.sdCardPendingMountFolder.c_str());
        ImGui::Spacing();
        ImGui::TextUnformatted("Without this file, file deletions on the host will be restricted to prevent accidental data loss.");
        ImGui::Spacing();
        if (ImGui::Button("Create .65C02_SIM_mount & Allow Deletion")) {
            const auto& pending = state.popups.sdCardPendingMountFolder;
            GUI::CreateMountSentinel(pending);
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
    if (state.popups.showCartridgeLoaded && (!ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId) || ImGui::IsPopupOpen("Cartridge Loaded"))) {
        ImGui::OpenPopup("Cartridge Loaded");
        state.popups.showCartridgeLoaded = false;
    }

    ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5F, 0.5F));

    if (ImGui::BeginPopupModal("Cartridge Loaded", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
        static const Core::Cartridge emptyCart;
        const auto& cart = state.backend ? state.backend->GetCartridge() : emptyCart;
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

    if (state.popups.showCartridgeSavePrompt && (!ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId) || ImGui::IsPopupOpen("Save SD Card Changes?###CartridgeSDSavePrompt"))) {
        ImGui::OpenPopup("Save SD Card Changes?###CartridgeSDSavePrompt");
        state.popups.showCartridgeSavePrompt = false;
    }

    ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5F, 0.5F));
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

bool ShouldPromptCartridgeSDSave(const Control::AppState& state) {
    if (!state.backend) {
        return false;
    }
    const auto& cart = state.backend->GetCartridge();
    return cart.loaded && !cart.sdCardPath.empty() && state.backend->GetSDCard().IsMounted() &&
           state.backend->GetSDCard().IsModified();
}

void ExecuteReset(Control::AppState& state) {
    bool wasRunning = state.backend && !state.backend->IsPaused();

    if (state.backend && state.backend->GetCartridge().loaded && !state.backend->GetCartridge().sdCardPath.empty() &&
        state.backend->GetSDCard().IsMounted() && state.backend->GetSDCard().IsModified()) {
        Core::CartridgeLoader::SaveSDToZip(state.backend->GetCartridge());
        state.backend->GetSDCard().SetModified(false);
    }

    static const Core::Cartridge emptyCart;
    Core::Cartridge cart = state.backend ? state.backend->GetCartridge() : emptyCart;
    bool hadCartridge = cart.loaded;
    std::string romBin = state.rom.bin;
    std::vector<uint8_t> romData = state.rom.data;
    bool hadRom = state.rom.loaded;

    state.RecreateBackend(state.cachedSidSampleRate, /*reloadRom=*/false);

    Console::Clear();
    if (state.crtFilter.IsInitialized()) {
        state.crtFilter.Reset();
    }
    state.render.lastDisplayTex = 0;

    if (state.rom.symbols.GetSymbolCount() > 0 && state.backend) {
        state.backend->GetSubroutineProfiler().SetSymbolTable(&state.rom.symbols);
    }

    if (hadCartridge) {
#ifndef TARGET_WASM
        if (!cart.sourceZipPath.empty() && std::filesystem::exists(cart.sourceZipPath)) {
            std::string errorMsg;
            Core::Cartridge freshCart;
            if (Core::CartridgeLoader::Load(cart.sourceZipPath, freshCart, errorMsg)) {
                cart = std::move(freshCart);
            }
        }
#endif
        Control::ApplyCartridgeConfig(state, cart);
        std::string errorMsg;
        if (state.backend) {
            state.backend->InitFromMemory(cart.romData, cart.romFileName, errorMsg);
        }
    } else if (hadRom) {
        std::string errorMsg;
#ifdef TARGET_WASM
        if (state.backend && !state.backend->InitFromMemory(romData, romBin, errorMsg)) {
            state.rom.loaded = false;
        }
#else
        bool initSuccess = false;
        if (!romData.empty() && state.backend) {
            initSuccess = state.backend->InitFromMemory(romData, romBin, errorMsg);
        } else if (!romBin.empty() && state.backend) {
            initSuccess = state.backend->Init(romBin, errorMsg);
        }
        if (!initSuccess) {
            state.rom.loaded = false;
            Frontend::CustomFileDialog::OpenDialog("ChooseFileDlgKey", "Choose File", "Supported Files (*.bin, *.65c){.bin,.65c,.c65},.bin,.65c,.*", ".");
        }
#endif
    }

    if (state.backend) {
        state.backend->Start();

        if (wasRunning) {
            state.backend->Resume();
            state.backend->GetSID().SetEmulationPaused(false);
        }
    }
}

void ExecuteEjectCartridge(Control::AppState& state) {
    if (state.backend) {
        state.backend->GetSDCard().Unmount();
        state.backend->ClearCartridge();
        state.backend->SetGPUEnabled(false);
        state.backend->SetESPEnabled(false);
        state.backend->SetSDEnabled(false);
        state.backend->SetCycleAccurate(true);
        state.backend->SetTargetIPS(1000000);
        state.backend->Reset();
        state.backend->Pause();
    }
    state.emulation.gpuEnabled = false;
    state.emulation.espEnabled = false;
    state.emulation.sdEnabled = false;
    state.emulation.cycleAccurate = true;
    state.emulation.instructionsPerFrame = 1000000;
    state.rom.symbols.Clear();
    state.rom.loaded = false;
    state.rom.bin = "";
}

void PromptOrExecuteCartridgeAction(Control::AppState& state, Control::PendingCartridgeAction action) {
    if (Frontend::ShouldPromptCartridgeSDSave(state)) {
        if (state.backend) {
            state.backend->Pause();
        }
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
    if (saveChanges && state.backend) {
        const auto& cart = state.backend->GetCartridge();
        if (cart.loaded && !cart.sdCardPath.empty()) {
            Core::CartridgeLoader::SaveSDToZip(cart);
        }
    }
    if (state.backend) {
        state.backend->GetSDCard().SetModified(false);
    }

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
