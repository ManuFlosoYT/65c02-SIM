#pragma once

#include "../../CPU.h"
#include "../../Mem.h"

namespace Hardware::Instructions {

class CPY {
public:
    static void ExecuteImmediate(CPU& cpu, Mem& mem);
    static void ExecuteZP(CPU& cpu, Mem& mem);
    static void ExecuteABS(CPU& cpu, Mem& mem);
};

}  // namespace Hardware::Instructions
