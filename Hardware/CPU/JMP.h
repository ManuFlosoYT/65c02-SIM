#pragma once

#include "../CPU.h"
#include "../Mem.h"

namespace Hardware::Instructions {

class JMP {
public:
    static void ExecuteABS(CPU& cpu, Mem& mem);
    static void ExecuteIND(CPU& cpu, Mem& mem);
    static void ExecuteABSX(CPU& cpu, Mem& mem);
};

}  // namespace Hardware::Instructions
