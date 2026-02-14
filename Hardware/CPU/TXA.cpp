#include "TXA.h"

namespace Hardware::Instructions {

void TXA::Execute(CPU& cpu, Mem& mem) {
    cpu.A = cpu.X;
    cpu.Z = (cpu.A == 0);
    cpu.N = (cpu.A & 0b10000000) > 0;
}

}  // namespace Hardware::Instructions
