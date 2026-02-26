#include "BCS.h"
#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"
#include "InstructionSet.h"

namespace Hardware::Instructions {

void BCS::Execute(CPU& cpu, Bus& bus) {
    int8_t dir = cpu.FetchByte(bus);
    if(cpu.C == 1) {
        cpu.PC += dir;
        cpu.UpdatePagePtr(bus);
    }
}

}  // namespace Hardware::Instructions
