#include "PHY.h"
#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"
#include "InstructionSet.h"

namespace Hardware::Instructions {

void PHY::Execute(CPU& cpu, Bus& bus) {
    cpu.PushByte(cpu.Y, bus);
}

}  // namespace Hardware::Instructions
