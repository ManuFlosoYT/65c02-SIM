#include "JMP.h"

void JMP::ExecuteABS(CPU& cpu, Mem& mem) {
    Word dir = cpu.FetchWord(mem);
    cpu.PC = dir;
}

void JMP::ExecuteABSX(CPU& cpu, Mem& mem) {
    Word dir = cpu.FetchWord(mem);
    dir += cpu.X;
    cpu.PC = dir;
}

void JMP::ExecuteIND(CPU& cpu, Mem& mem) {
    Word dirIND = cpu.FetchWord(mem);
    Word dir = cpu.ReadWord(dirIND, mem);
    cpu.PC = dir;
}