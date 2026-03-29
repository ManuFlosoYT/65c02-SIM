#include "Frontend/Control/CartridgeUtils.h"
#include "Frontend/GUI/Video/VRAMViewerWindow.h"
#include "Hardware/Core/CartridgeLoader.h"

namespace Control {

void ApplyCartridgeConfig(AppState& state, const Core::Cartridge& cart) {
    // Save previous cartridge SD if it exists
    if (state.emulator.GetCartridge().loaded && !state.emulator.GetCartridge().sdCardPath.empty()) {
        Core::CartridgeLoader::SaveSDToZip(state.emulator.GetCartridge());
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
    if (cart.config.sidEnabled.value_or(false)) {
        state.emulator.GetSID().EnableSound(true);
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

} // namespace Control
