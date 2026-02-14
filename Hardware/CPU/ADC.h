#pragma once

#include "../CPU.h"
#include "../Mem.h"

namespace Hardware::Instructions {

class ADC {
public:
    static void ExecuteImmediate(CPU& cpu, Mem& mem);
    static void ExecuteZP(CPU& cpu, Mem& mem);
    static void ExecuteZPX(CPU& cpu, Mem& mem);
    static void ExecuteABS(CPU& cpu, Mem& mem);
    static void ExecuteABSX(CPU& cpu, Mem& mem);
    static void ExecuteABSY(CPU& cpu, Mem& mem);
    static void ExecuteINDX(CPU& cpu, Mem& mem);
    static void ExecuteINDY(CPU& cpu, Mem& mem);
    static void ExecuteIND_ZP(CPU& cpu, Mem& mem);
};

}  // namespace Hardware::Instructions
