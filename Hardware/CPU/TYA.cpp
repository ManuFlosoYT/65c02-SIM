#include "TYA.h"

void TYA::Execute(CPU& cpu, Mem& mem) {
    cpu.A = cpu.Y;
    cpu.Z = (cpu.A == 0);
    cpu.N = (cpu.A & 0b10000000) > 0;
}
