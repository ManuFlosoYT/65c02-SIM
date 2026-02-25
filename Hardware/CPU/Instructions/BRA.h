#pragma once

#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"

namespace Hardware::Instructions {

class BRA {
public:
    static void Execute(CPU& cpu, Bus& bus);
};

}  // namespace Hardware::Instructions
