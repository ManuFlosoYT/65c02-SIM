#include "BEQ.h"

void BEQ::Execute(CPU& cpu, Mem& mem) {
    int8_t dir = cpu.FetchByte(mem);
    if(cpu.Z == 1) {
        cpu.PC += dir;
    }
}
