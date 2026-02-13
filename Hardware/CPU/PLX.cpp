#include "PLX.h"

void PLX::Execute(CPU& cpu, Mem& mem) {
    cpu.X = cpu.PopByte(mem);
    cpu.Z = (cpu.X == 0);
    cpu.N = (cpu.X & 0b10000000) > 0;
}
