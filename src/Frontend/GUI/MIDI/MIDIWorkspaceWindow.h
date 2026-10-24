#pragma once

#include <imgui.h>
#include <string>
#include "Frontend/Control/AppState.h"

namespace GUI {

void DrawMIDIWorkspaceWindow(Control::AppState& state, ImVec2 workPos, ImVec2 workSize, float topSectionHeight,
                             ImGuiWindowFlags windowFlags);

void OpenMIDIFileInWorkspace(Control::AppState& state, const std::string& filePath);

bool IsMIDIAudioActive();

}  // namespace GUI

