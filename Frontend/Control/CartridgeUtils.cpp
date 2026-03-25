#include "Frontend/Control/CartridgeUtils.h"
#include "Frontend/GUI/Video/VRAMViewerWindow.h"

namespace Control {

void ApplyCartridgeConfig(AppState& state, const Core::Cartridge& cart) {
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
    
    // Refresh VRAM texture if an image was loaded
    if (!cart.vramData.empty() && state.emulation.gpuEnabled) {
        ::GUI::ForceRefreshVRAM(state);
    }
}

} // namespace Control
