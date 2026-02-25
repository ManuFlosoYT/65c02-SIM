#include "SED.h"
#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"
#include "InstructionSet.h"

namespace Hardware::Instructions {

void SED::Execute(CPU& cpu, Bus& bus) {
    cpu.D = 1;
}

}  // namespace Hardware::Instructions
