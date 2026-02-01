#include "JMP.h"

void JMP::EjecutarABS(CPU& cpu, Mem& mem) {
    Word dir = cpu.FetchWord(mem);
    cpu.PC = dir;
}

void JMP::EjecutarABSX(CPU& cpu, Mem& mem) {
    Word dir = cpu.FetchWord(mem);
    dir += cpu.X;
    cpu.PC = dir;
}

void JMP::EjecutarIND(CPU& cpu, Mem& mem) {
    Word dirIND = cpu.FetchWord(mem);
    Word dir = cpu.LeerWord(dirIND, mem);
    cpu.PC = dir;
}