#include "BCC.h"

void BCC::Execute(CPU& cpu, Mem& mem) {
    int8_t dir = cpu.FetchByte(mem);
    if(cpu.C == 0) {
        cpu.PC += dir;
    }
}
