#include "BRA.h"
#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"
#include "InstructionSet.h"

namespace Hardware::Instructions {

void BRA::Execute(CPU& cpu, Bus& bus) {
    int8_t offset = cpu.FetchByte(bus);
    cpu.PC += offset;
    cpu.UpdatePagePtr(bus);
}

}  // namespace Hardware::Instructions
