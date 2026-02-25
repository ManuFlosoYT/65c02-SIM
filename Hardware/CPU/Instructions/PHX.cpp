#include "PHX.h"
#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"
#include "InstructionSet.h"

namespace Hardware::Instructions {

void PHX::Execute(CPU& cpu, Bus& bus) {
    cpu.PushByte(cpu.X, bus);
}

}  // namespace Hardware::Instructions
