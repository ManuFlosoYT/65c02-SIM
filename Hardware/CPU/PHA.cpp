#include "PHA.h"

void PHA::Execute(CPU& cpu, Mem& mem) {
    cpu.PushByte(cpu.A, mem);
}
