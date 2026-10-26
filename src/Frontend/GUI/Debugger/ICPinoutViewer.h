#pragma once

#include "Hardware/Core/LogicAnalyzer.h"

namespace GUI {

class ICPinoutViewer {
public:
    static void Draw(int& selectedChip, const Hardware::LogicSample& sample,
                     float availWidth, float availHeight);
};

}  // namespace GUI
