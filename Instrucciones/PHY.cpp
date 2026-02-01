#include "PHY.h"

void PHY::Ejecutar(CPU& cpu, Mem& mem) {
    cpu.PushByte(cpu.Y, mem);
}
