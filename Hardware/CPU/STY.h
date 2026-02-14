#pragma once

#include "../CPU.h"
#include "../Mem.h"

namespace Hardware::Instructions {

class STY {
public:
    static void ExecuteZP(CPU& cpu, Mem& mem);
    static void ExecuteZPX(CPU& cpu, Mem& mem);
    static void ExecuteABS(CPU& cpu, Mem& mem);
};

}  // namespace Hardware::Instructions
