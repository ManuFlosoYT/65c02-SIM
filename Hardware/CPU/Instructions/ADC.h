#pragma once

#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"

namespace Hardware::Instructions {

class ADC {
public:
    static void ExecuteImmediate(CPU& cpu, Bus& bus);
    static void ExecuteZP(CPU& cpu, Bus& bus);
    static void ExecuteZPX(CPU& cpu, Bus& bus);
    static void ExecuteABS(CPU& cpu, Bus& bus);
    static void ExecuteABSX(CPU& cpu, Bus& bus);
    static void ExecuteABSY(CPU& cpu, Bus& bus);
    static void ExecuteINDX(CPU& cpu, Bus& bus);
    static void ExecuteINDY(CPU& cpu, Bus& bus);
    static void ExecuteIND_ZP(CPU& cpu, Bus& bus);
};

}  // namespace Hardware::Instructions
