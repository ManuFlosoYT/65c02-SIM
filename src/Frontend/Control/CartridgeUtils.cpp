#include "Frontend/Control/CartridgeUtils.h"
#include "Hardware/Core/Console.h"
#include "Frontend/GUI/Video/VRAMViewerWindow.h"
#include "Hardware/Core/CartridgeLoader.h"

namespace Control {

void ApplyCartridgeConfig(AppState& state, const Core::Cartridge& cart) {
    Console::Clear();
    if (!state.backend) {
        return;
    }
    if (state.backend->GetCartridge().loaded && !state.backend->GetCartridge().sdCardPath.empty() &&
        state.backend->GetSDCard().IsModified()) {
        Core::CartridgeLoader::SaveSDToZip(state.backend->GetCartridge());
        state.backend->GetSDCard().SetModified(false);
    }

    state.backend->SetCartridge(cart);

    state.emulation.gpuEnabled = cart.config.gpuEnabled.value_or(false);
    state.emulation.instructionsPerFrame = cart.config.targetIPS.value_or(1000000);
    state.emulation.cycleAccurate = cart.config.cycleAccurate.value_or(true);
    state.emulation.sdEnabled = cart.config.sdEnabled.value_or(false);
    state.emulation.espEnabled = cart.config.espEnabled.value_or(false);
    state.backend->GetSID().EnableSound(cart.config.sidEnabled.value_or(true));
    state.backend->GetSID().SetModel(cart.config.sidModel.value_or(Hardware::SIDModel::MOS8580));
    
    state.backend->SetTargetIPS(state.emulation.instructionsPerFrame);
    state.backend->SetGPUEnabled(state.emulation.gpuEnabled);
    state.backend->SetSDEnabled(state.emulation.sdEnabled);
    state.backend->SetESPEnabled(state.emulation.espEnabled);
    state.backend->SetCycleAccurate(state.emulation.cycleAccurate);
    state.backend->ClearProfiler();

    if (!cart.sdCardPath.empty()) {
        state.backend->GetSDCard().Mount(cart.sdCardPath);
    }
    
    if (!cart.vramData.empty() && state.emulation.gpuEnabled) {
        ::GUI::ForceRefreshVRAM(state);
    }

    state.rom.symbols.Clear();
    if (!cart.dbgFilePath.empty()) {
        state.rom.symbols.LoadFromFile(cart.dbgFilePath);
        state.backend->GetSubroutineProfiler().SetSymbolTable(&state.rom.symbols);
    }
}

} // namespace Control
