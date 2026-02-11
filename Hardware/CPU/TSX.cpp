#include "TSX.h"

void TSX::Ejecutar(CPU& cpu, Mem& mem) {
    cpu.X = cpu.SP;
    cpu.Z = (cpu.X == 0);
    cpu.N = (cpu.X & 0b10000000) > 0;
}
