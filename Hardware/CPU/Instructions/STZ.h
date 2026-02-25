#pragma once

#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"

namespace Hardware::Instructions {

class STZ {
public:
    static void ExecuteZP(CPU& cpu, Bus& bus);
    static void ExecuteZPX(CPU& cpu, Bus& bus);
    static void ExecuteABS(CPU& cpu, Bus& bus);
    static void ExecuteABSX(CPU& cpu, Bus& bus);
};

}  // namespace Hardware::Instructions
