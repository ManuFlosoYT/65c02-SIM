#include "PHP.h"

namespace Hardware::Instructions {

void PHP::Execute(CPU& cpu, Bus& bus) {    
    cpu.PushByte(cpu.GetStatus(), bus);
}

}  // namespace Hardware::Instructions
