#pragma once

#include "Frontend/Control/AppState.h"

namespace Frontend {

void HandleDialogs(Control::AppState& state);
void HandleCreateSDDialog(Control::AppState& state);
void HandleMountFolderDialog(Control::AppState& state);
void HandleLoadScriptDialog(Control::AppState& state);
void HandleMountSDDialog(Control::AppState& state);

}  // namespace Frontend
