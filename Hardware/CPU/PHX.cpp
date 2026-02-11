#include "PHX.h"

void PHX::Ejecutar(CPU& cpu, Mem& mem) {
    cpu.PushByte(cpu.X, mem);
}
