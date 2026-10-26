#pragma once

#include "Frontend/Control/AppState.h"
#include "Hardware/Core/SubroutineProfiler.h"
#include <string>

namespace GUI {

class FlameGraphView {
public:
    static void Draw(Control::AppState& state, const Hardware::SubroutineProfiler::Snapshot& snapshot);
    static bool MatchesFilter(const std::string& name, uint16_t addr, const char* filter);
    static std::string FormatHex16(uint16_t addr);
    static char* GetFilter();
    static void ClearFilter();
};

}  // namespace GUI
