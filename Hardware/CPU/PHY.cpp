#include "PHY.h"

void PHY::Execute(CPU& cpu, Mem& mem) {
    cpu.PushByte(cpu.Y, mem);
}
