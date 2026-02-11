#include "TAY.h"

void TAY::Ejecutar(CPU& cpu, Mem& mem) {
    cpu.Y = cpu.A;
    cpu.Z = (cpu.Y == 0);
    cpu.N = (cpu.Y & 0b10000000) > 0;
}
