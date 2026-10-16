#include "Frontend/Control/CartridgeUtils.h"
#include <iostream>
#ifndef TARGET_WASM
#include "Frontend/UI/CustomFileDialog.h"
#endif
#include "Frontend/Control/Console.h"
#include "Frontend/GUI/Video/VRAMViewerWindow.h"
#include "Hardware/Core/CartridgeLoader.h"

namespace Control {

void ApplyCartridgeConfig(AppState& state, const Core::Cartridge& cart) {
    Console::Clear();
    if (state.emulator.GetCartridge().loaded && !state.emulator.GetCartridge().sdCardPath.empty() &&
        state.emulator.GetSDCard().IsModified()) {
        Core::CartridgeLoader::SaveSDToZip(state.emulator.GetCartridge());
        state.emulator.GetSDCard().SetModified(false);
    }

    state.emulator.SetCartridge(cart);

    if (cart.config.gpuEnabled.has_value()) {
        state.emulation.gpuEnabled = cart.config.gpuEnabled.value();
    }
    if (cart.config.targetIPS.has_value()) {
        state.emulation.instructionsPerFrame = cart.config.targetIPS.value();
    }
    if (cart.config.cycleAccurate.has_value()) {
        state.emulation.cycleAccurate = cart.config.cycleAccurate.value();
    }
    if (cart.config.sdEnabled.has_value()) {
        state.emulation.sdEnabled = cart.config.sdEnabled.value();
    }
    if (cart.config.espEnabled.has_value()) {
        state.emulation.espEnabled = cart.config.espEnabled.value();
    }
    
    // Explicit hardware activation
    if (cart.config.sidEnabled.has_value()) {
        state.emulator.GetSID().EnableSound(cart.config.sidEnabled.value());
    }
    if (cart.config.sidModel.has_value()) {
        state.emulator.GetSID().SetModel(cart.config.sidModel.value());
    }
    
    state.emulator.SetTargetIPS(state.emulation.instructionsPerFrame);
    state.emulator.SetGPUEnabled(state.emulation.gpuEnabled);
    state.emulator.SetSDEnabled(state.emulation.sdEnabled);
    state.emulator.SetESPEnabled(state.emulation.espEnabled);
    state.emulator.SetCycleAccurate(state.emulation.cycleAccurate);
    state.emulator.ClearProfiler();

    // Automatic SD mounting for v3 cartridges
    if (!cart.sdCardPath.empty()) {
        state.emulator.GetSDCard().Mount(cart.sdCardPath);
    }
    
    // Refresh VRAM texture if an image was loaded
    if (!cart.vramData.empty() && state.emulation.gpuEnabled) {
        ::GUI::ForceRefreshVRAM(state);
    }
}

bool ShouldPromptCartridgeSDSave(const AppState& state) {
    const auto& cart = state.emulator.GetCartridge();
    return cart.loaded && !cart.sdCardPath.empty() && state.emulator.GetSDCard().IsMounted() &&
           state.emulator.GetSDCard().IsModified();
}

void ExecuteReset(AppState& state) {
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
        } else {
            state.emulator.SetGPUEnabled(state.emulation.gpuEnabled);
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
            Frontend::CustomFileDialog::OpenDialog("ChooseFileDlgKey", "Choose File", ".bin", ".");
        } else {
            state.emulator.SetGPUEnabled(state.emulation.gpuEnabled);
        }
#endif
    }

    if (wasRunning) {
        state.emulator.Resume();
        state.emulator.GetSID().SetEmulationPaused(false);
    }
}

void ExecuteEjectCartridge(AppState& state) {
    state.emulator.GetSDCard().Unmount();
    state.emulator.ClearCartridge();
    state.rom.loaded = false;
    state.rom.bin = "";
    state.emulator.Reset();
    state.emulator.Pause();
}

void PromptOrExecuteCartridgeAction(AppState& state, PendingCartridgeAction action) {
    if (ShouldPromptCartridgeSDSave(state)) {
        state.emulator.Pause();
        state.popups.showCartridgeSavePrompt = true;
        state.popups.pendingCartridgeAction = action;
        return;
    }

    switch (action) {
        case PendingCartridgeAction::Reset:
            ExecuteReset(state);
            break;
        case PendingCartridgeAction::Eject:
            ExecuteEjectCartridge(state);
            break;
        case PendingCartridgeAction::Exit:
            state.requestExit = true;
            break;
        case PendingCartridgeAction::None:
            break;
    }
}

void ConfirmCartridgeAction(AppState& state, bool saveChanges) {
    if (saveChanges) {
        const auto& cart = state.emulator.GetCartridge();
        if (cart.loaded && !cart.sdCardPath.empty()) {
            Core::CartridgeLoader::SaveSDToZip(cart);
        }
    }
    state.emulator.GetSDCard().SetModified(false);

    auto action = state.popups.pendingCartridgeAction;
    state.popups.pendingCartridgeAction = PendingCartridgeAction::None;

    switch (action) {
        case PendingCartridgeAction::Reset:
            ExecuteReset(state);
            break;
        case PendingCartridgeAction::Eject:
            ExecuteEjectCartridge(state);
            break;
        case PendingCartridgeAction::Exit:
            state.requestExit = true;
            break;
        case PendingCartridgeAction::None:
            break;
    }
}

} // namespace Control
