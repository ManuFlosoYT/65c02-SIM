#include "PLA.h"
#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"
#include "InstructionSet.h"

namespace Hardware::Instructions {

void PLA::Execute(CPU& cpu, Bus& bus) {
    cpu.A = cpu.PopByte(bus);
    cpu.Z = (cpu.A == 0);
    cpu.N = (cpu.A & 0b10000000) > 0;
}

}  // namespace Hardware::Instructions
