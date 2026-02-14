#include "TAX.h"

namespace Hardware::Instructions {

void TAX::Execute(CPU& cpu, Mem& mem) {
    cpu.X = cpu.A;
    cpu.Z = (cpu.X == 0);
    cpu.N = (cpu.X & 0b10000000) > 0;
}

}  // namespace Hardware::Instructions
