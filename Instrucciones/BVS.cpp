#include "BVS.h"

void BVS::Ejecutar(CPU& cpu, Mem& mem) {
    int8_t dir = cpu.FetchByte(mem);
    if(cpu.V == 1) {
        cpu.PC += dir;
    }
}
