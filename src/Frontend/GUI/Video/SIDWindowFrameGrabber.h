#pragma once

#include "Frontend/Control/AppState.h"

namespace GUI {

class SIDWindowFrameGrabber {
public:
    static void CaptureSIDWindow(Control::AppState& state);
};

void CaptureSIDWindow(Control::AppState& state);

}  // namespace GUI

namespace Frontend {
using GUI::SIDWindowFrameGrabber;
using GUI::CaptureSIDWindow;
}  // namespace Frontend
