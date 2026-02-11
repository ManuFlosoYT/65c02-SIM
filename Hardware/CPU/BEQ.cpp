#include "BEQ.h"

void BEQ::Ejecutar(CPU& cpu, Mem& mem) {
    int8_t dir = cpu.FetchByte(mem);
    if(cpu.Z == 1) {
        cpu.PC += dir;
    }
}
