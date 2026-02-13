#include "BNE.h"

void BNE::Execute(CPU& cpu, Mem& mem) {
    int8_t dir = cpu.FetchByte(mem);
    if(cpu.Z == 0) {
        cpu.PC += dir;
    }
}
