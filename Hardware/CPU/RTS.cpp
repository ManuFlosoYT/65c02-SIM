#include "RTS.h"

void RTS::Execute(CPU& cpu, Mem& mem) {
    Word dir = cpu.PopWord(mem) + 1;
    cpu.PC = dir;
}
