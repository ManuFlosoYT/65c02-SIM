#pragma once

#include <imgui.h>
#include "Frontend/Control/AppState.h"
#include "Frontend/MediaExporter.h"
#include <memory>

namespace Frontend {
    void HandleDialogs(Control::AppState& state);
    void DrawPopups(Control::AppState& state);
    void DrawGUIWindows(Control::AppState& state, const ImVec2& work_pos, const ImVec2& work_size, float top_section_height, unsigned int windowFlags);
    void UpdateMediaRecording(Control::AppState& state, std::unique_ptr<MediaExporter>& mediaExporter);
}
