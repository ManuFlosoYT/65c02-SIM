#include "RTI.h"
#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"
#include "InstructionSet.h"

namespace Hardware::Instructions {

void RTI::Execute(CPU& cpu, Bus& bus) {
    Byte status = cpu.PopByte(bus);
    cpu.SetStatus(status);
    cpu.PC = cpu.PopWord(bus);
}

}  // namespace Hardware::Instructions
