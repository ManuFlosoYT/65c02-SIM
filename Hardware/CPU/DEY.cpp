#include "DEY.h"

void DEY::Execute(CPU& cpu, Mem& mem) {
    cpu.Y--;
    cpu.Z = (cpu.Y == 0);
    cpu.N = (cpu.Y & 0b10000000) > 0;
}
