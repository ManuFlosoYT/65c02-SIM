#pragma once

#include <imgui.h>
#include "Frontend/Control/AppState.h"
#include "Frontend/MediaExporter.h"
#include "Frontend/GUI/FileDialogs.h"
#include "Frontend/GUI/PopupManager.h"
#include "Frontend/GUI/RecordingManager.h"
#include <memory>

namespace Frontend {
    void DrawGUIWindows(Control::AppState& state, const ImVec2& work_pos, const ImVec2& work_size, float top_section_height, unsigned int windowFlags);
}
