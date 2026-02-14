#pragma once

#include "../CPU.h"
#include "../Mem.h"

namespace Hardware::Instructions {

class STX {
public:
    static void ExecuteZP(CPU& cpu, Mem& mem);
    static void ExecuteZPY(CPU& cpu, Mem& mem);
    static void ExecuteABS(CPU& cpu, Mem& mem);
};

}  // namespace Hardware::Instructions
