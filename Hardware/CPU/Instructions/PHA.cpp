#include "PHA.h"

namespace Hardware::Instructions {

void PHA::Execute(CPU& cpu, Mem& mem) {
    cpu.PushByte(cpu.A, mem);
}

}  // namespace Hardware::Instructions
