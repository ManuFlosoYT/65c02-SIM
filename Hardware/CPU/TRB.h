#pragma once

#include "../CPU.h"
#include "../Mem.h"

namespace Hardware::Instructions {

class TRB {
public:
    static void ExecuteZP(CPU& cpu, Mem& mem);
    static void ExecuteABS(CPU& cpu, Mem& mem);
};

}  // namespace Hardware::Instructions
