#include "BPL.h"

void BPL::Execute(CPU& cpu, Mem& mem) {
    int8_t dir = cpu.FetchByte(mem);
    if(cpu.N == 0) {
        cpu.PC += dir;
    }
}
