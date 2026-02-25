#pragma once

#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"

namespace Hardware::Instructions {

class JMP {
public:
    static void ExecuteABS(CPU& cpu, Bus& bus);
    static void ExecuteIND(CPU& cpu, Bus& bus);
    static void ExecuteABSX(CPU& cpu, Bus& bus);
};

}  // namespace Hardware::Instructions
