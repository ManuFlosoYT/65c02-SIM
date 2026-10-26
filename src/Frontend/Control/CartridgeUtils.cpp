#include "Frontend/Control/CartridgeUtils.h"
#include "Hardware/Core/Console.h"
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

    state.emulation.gpuEnabled = cart.config.gpuEnabled.value_or(false);
    state.emulation.instructionsPerFrame = cart.config.targetIPS.value_or(1000000);
    state.emulation.cycleAccurate = cart.config.cycleAccurate.value_or(true);
    state.emulation.sdEnabled = cart.config.sdEnabled.value_or(false);
    state.emulation.espEnabled = cart.config.espEnabled.value_or(false);
    state.emulator.GetSID().EnableSound(cart.config.sidEnabled.value_or(true));
    state.emulator.GetSID().SetModel(cart.config.sidModel.value_or(Hardware::SIDModel::MOS8580));
    
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

    state.rom.symbols.Clear();
    if (!cart.dbgFilePath.empty()) {
        state.rom.symbols.LoadFromFile(cart.dbgFilePath);
        state.emulator.GetSubroutineProfiler().SetSymbolTable(&state.rom.symbols);
    }
}

} // namespace Control
