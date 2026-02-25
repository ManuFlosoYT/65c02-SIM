#pragma once

#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"

namespace Hardware::Instructions {

class LDX {
public:
    static void ExecuteImmediate(CPU& cpu, Bus& bus);
    static void ExecuteZP(CPU& cpu, Bus& bus);
    static void ExecuteZPY(CPU& cpu, Bus& bus);
    static void ExecuteABS(CPU& cpu, Bus& bus);
    static void ExecuteABSY(CPU& cpu, Bus& bus);
};

}  // namespace Hardware::Instructions
