#include "CLC.h"
#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"
#include "InstructionSet.h"

namespace Hardware::Instructions {

void CLC::Execute(CPU& cpu, Bus& bus) {
    cpu.C = 0;
}

}  // namespace Hardware::Instructions
