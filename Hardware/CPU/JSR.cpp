#include "JSR.h"

void JSR::Execute(CPU& cpu, Mem& mem) {
    Word Dir = cpu.FetchWord(mem);
    Word RetDir = cpu.PC - 1;

    cpu.PushWord(RetDir, mem);

    cpu.PC = Dir;
}
