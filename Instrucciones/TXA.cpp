#include "TXA.h"

void TXA::Ejecutar(CPU& cpu, Mem& mem) {
    cpu.A = cpu.X;
    cpu.Z = (cpu.A == 0);
    cpu.N = (cpu.A & 0b10000000) > 0;
}
