#include "RTS.h"

void RTS::Ejecutar(CPU& cpu, Mem& mem) {
    Word dir = cpu.PopWord(mem) + 1;
    cpu.PC = dir;
}
