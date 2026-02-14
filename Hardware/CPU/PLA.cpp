#include "PLA.h"

namespace Hardware::Instructions {

void PLA::Execute(CPU& cpu, Mem& mem) {
    cpu.A = cpu.PopByte(mem);
    cpu.Z = (cpu.A == 0);
    cpu.N = (cpu.A & 0b10000000) > 0;
}

}  // namespace Hardware::Instructions
