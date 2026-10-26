#pragma once

#include "Frontend/Control/AppState.h"
#include "Frontend/GUI/PopupManager.h"
#include "Hardware/Core/Cartridge.h"

namespace Control {

/**
 * Applies a cartridge's configuration to the global AppState and Emulator.
 * Handles IPS, GPU enablement, Cycle Accuracy, SID, etc.
 */
void ApplyCartridgeConfig(AppState& state, const Core::Cartridge& cart);

inline bool ShouldPromptCartridgeSDSave(const AppState& state) {
    return Frontend::ShouldPromptCartridgeSDSave(state);
}

inline void ExecuteReset(AppState& state) {
    Frontend::ExecuteReset(state);
}

inline void ExecuteEjectCartridge(AppState& state) {
    Frontend::ExecuteEjectCartridge(state);
}

inline void PromptOrExecuteCartridgeAction(AppState& state, PendingCartridgeAction action) {
    Frontend::PromptOrExecuteCartridgeAction(state, action);
}

inline void ConfirmCartridgeAction(AppState& state, bool saveChanges) {
    Frontend::ConfirmCartridgeAction(state, saveChanges);
}

} // namespace Control
