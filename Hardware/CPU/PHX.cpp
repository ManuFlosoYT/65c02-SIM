#include "PHX.h"

void PHX::Execute(CPU& cpu, Mem& mem) {
    cpu.PushByte(cpu.X, mem);
}
