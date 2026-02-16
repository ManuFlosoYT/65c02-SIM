#include "PHP.h"

namespace Hardware::Instructions {

void PHP::Execute(CPU& cpu, Mem& mem) {    
    cpu.PushByte(cpu.GetStatus(), mem);
}

}  // namespace Hardware::Instructions
