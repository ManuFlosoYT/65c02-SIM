#pragma once

#include "Frontend/Control/AppState.h"
#include "Hardware/Core/Cartridge.h"

namespace Control {

/**
 * Applies a cartridge's configuration to the global AppState and Emulator.
 * Handles IPS, GPU enablement, Cycle Accuracy, SID, etc.
 */
void ApplyCartridgeConfig(AppState& state, const Core::Cartridge& cart);

bool ShouldPromptCartridgeSDSave(const AppState& state);
void ExecuteReset(AppState& state);
void ExecuteEjectCartridge(AppState& state);
void PromptOrExecuteCartridgeAction(AppState& state, PendingCartridgeAction action);
void ConfirmCartridgeAction(AppState& state, bool saveChanges);

} // namespace Control
