#include "BMI.h"
#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"
#include "InstructionSet.h"

namespace Hardware::Instructions {

void BMI::Execute(CPU& cpu, Bus& bus) {
    int8_t dir = cpu.FetchByte(bus);
    if(cpu.N == 1) {
        cpu.PC += dir;
        cpu.UpdatePagePtr(bus);
    }
}

}  // namespace Hardware::Instructions
