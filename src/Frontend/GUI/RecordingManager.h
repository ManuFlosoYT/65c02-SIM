#pragma once

#include <memory>
#include "Frontend/Control/AppState.h"
#include "Frontend/MediaExporter.h"

namespace Frontend {

void UpdateMediaRecording(Control::AppState& state, std::unique_ptr<MediaExporter>& mediaExporter);

}  // namespace Frontend

