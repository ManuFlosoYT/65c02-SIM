#include "BCS.h"

void BCS::Execute(CPU& cpu, Mem& mem) {
    int8_t dir = cpu.FetchByte(mem);
    if(cpu.C == 1) {
        cpu.PC += dir;
    }
}
