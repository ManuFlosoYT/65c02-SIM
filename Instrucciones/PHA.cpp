#include "PHA.h"

void PHA::Ejecutar(CPU& cpu, Mem& mem) {
    cpu.PushByte(cpu.A, mem);
}
