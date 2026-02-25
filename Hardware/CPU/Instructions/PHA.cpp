#include "PHA.h"
#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"
#include "InstructionSet.h"

namespace Hardware::Instructions {

void PHA::Execute(CPU& cpu, Bus& bus) {
    cpu.PushByte(cpu.A, bus);
}

}  // namespace Hardware::Instructions
