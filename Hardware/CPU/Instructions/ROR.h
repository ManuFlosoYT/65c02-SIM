#pragma once

#include "../../CPU.h"
#include "../../Mem.h"

namespace Hardware::Instructions {

class ROR {
public:
    static void ExecuteAccumulator(CPU& cpu, Mem& mem);
    static void ExecuteZP(CPU& cpu, Mem& mem);
    static void ExecuteZPX(CPU& cpu, Mem& mem);
    static void ExecuteABS(CPU& cpu, Mem& mem);
    static void ExecuteABSX(CPU& cpu, Mem& mem);
};

}  // namespace Hardware::Instructions
