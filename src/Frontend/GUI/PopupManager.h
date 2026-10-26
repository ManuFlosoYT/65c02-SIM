#pragma once

#include "Frontend/Control/AppState.h"

namespace Frontend {

void DrawPopups(Control::AppState& state);

bool ShouldPromptCartridgeSDSave(const Control::AppState& state);
void ExecuteReset(Control::AppState& state);
void ExecuteEjectCartridge(Control::AppState& state);
void PromptOrExecuteCartridgeAction(Control::AppState& state, Control::PendingCartridgeAction action);
void ConfirmCartridgeAction(Control::AppState& state, bool saveChanges);

}  // namespace Frontend

namespace GUI {

using Frontend::ShouldPromptCartridgeSDSave;
using Frontend::ExecuteReset;
using Frontend::ExecuteEjectCartridge;
using Frontend::PromptOrExecuteCartridgeAction;
using Frontend::ConfirmCartridgeAction;

}  // namespace GUI
