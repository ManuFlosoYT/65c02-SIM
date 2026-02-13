#include "TAX.h"

void TAX::Execute(CPU& cpu, Mem& mem) {
    cpu.X = cpu.A;
    cpu.Z = (cpu.X == 0);
    cpu.N = (cpu.X & 0b10000000) > 0;
}
