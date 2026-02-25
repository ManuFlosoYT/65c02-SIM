#pragma once

#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"

namespace Hardware::Instructions {

class PHY {
public:
    static void Execute(CPU& cpu, Bus& bus);
};

}  // namespace Hardware::Instructions
