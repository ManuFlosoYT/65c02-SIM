#pragma once

#include "../../CPU.h"
#include "../../Mem.h"

namespace Hardware::Instructions {

class STA {
public:
    static void ExecuteZP(CPU& cpu, Mem& mem);
    static void ExecuteZPX(CPU& cpu, Mem& mem);
    static void ExecuteABS(CPU& cpu, Mem& mem);
    static void ExecuteABSX(CPU& cpu, Mem& mem);
    static void ExecuteABSY(CPU& cpu, Mem& mem);
    static void ExecuteINDX(CPU& cpu, Mem& mem);
    static void ExecuteINDY(CPU& cpu, Mem& mem);
    static void ExecuteINDZP(CPU& cpu, Mem& mem);
};

}  // namespace Hardware::Instructions
