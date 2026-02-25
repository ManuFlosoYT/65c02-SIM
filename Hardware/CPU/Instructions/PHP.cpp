#include "PHP.h"
#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"
#include "InstructionSet.h"

namespace Hardware::Instructions {

void PHP::Execute(CPU& cpu, Bus& bus) {    
    cpu.PushByte(cpu.GetStatus(), bus);
}

}  // namespace Hardware::Instructions
