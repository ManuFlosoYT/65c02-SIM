#include "DEX.h"

void DEX::Execute(CPU& cpu, Mem& mem) {
    cpu.X--;
    cpu.Z = (cpu.X == 0);
    cpu.N = (cpu.X & 0b10000000) > 0;
}
