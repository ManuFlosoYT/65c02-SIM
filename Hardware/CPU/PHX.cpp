#include "PHX.h"

namespace Hardware::Instructions {

void PHX::Execute(CPU& cpu, Mem& mem) {
    cpu.PushByte(cpu.X, mem);
}

}  // namespace Hardware::Instructions
