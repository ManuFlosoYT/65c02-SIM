#include "SEC.h"
#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"
#include "InstructionSet.h"

namespace Hardware::Instructions {

void SEC::Execute(CPU& cpu, Bus& bus) {
    cpu.C = 1;
}

}  // namespace Hardware::Instructions
