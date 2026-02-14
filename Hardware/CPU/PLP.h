#pragma once

#include "../CPU.h"
#include "../Mem.h"

namespace Hardware::Instructions {

class PLP {
public:
    static void Execute(CPU& cpu, Mem& mem);
};

}  // namespace Hardware::Instructions
