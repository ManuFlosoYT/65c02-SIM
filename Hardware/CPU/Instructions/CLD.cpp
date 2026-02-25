#include "CLD.h"
#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"
#include "InstructionSet.h"

namespace Hardware::Instructions {

void CLD::Execute(CPU& cpu, Bus& bus) {
    cpu.D = 0;
}

}  // namespace Hardware::Instructions
