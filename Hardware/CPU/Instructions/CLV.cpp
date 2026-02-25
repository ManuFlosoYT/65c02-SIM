#include "CLV.h"
#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"
#include "InstructionSet.h"

namespace Hardware::Instructions {

void CLV::Execute(CPU& cpu, Bus& bus) {
    cpu.V = 0;
}

}  // namespace Hardware::Instructions
