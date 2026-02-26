#include "RTS.h"
#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"
#include "InstructionSet.h"

namespace Hardware::Instructions {

void RTS::Execute(CPU& cpu, Bus& bus) {
    Word dir = cpu.PopWord(bus) + 1;
    cpu.PC = dir;
    cpu.UpdatePagePtr(bus);
}

}  // namespace Hardware::Instructions
