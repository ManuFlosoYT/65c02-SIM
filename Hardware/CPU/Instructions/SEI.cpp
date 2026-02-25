#include "SEI.h"
#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"
#include "InstructionSet.h"

namespace Hardware::Instructions {

void SEI::Execute(CPU& cpu, Bus& bus) {
    cpu.I = 1;
}

}  // namespace Hardware::Instructions
