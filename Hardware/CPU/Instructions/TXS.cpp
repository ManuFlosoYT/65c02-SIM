#include "TXS.h"
#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"
#include "InstructionSet.h"

namespace Hardware::Instructions {

void TXS::Execute(CPU& cpu, Bus& bus) { 
    cpu.SP = 0x0100 | cpu.X; 
}

}  // namespace Hardware::Instructions
