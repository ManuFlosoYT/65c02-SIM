#include "BVC.h"

void BVC::Ejecutar(CPU& cpu, Mem& mem) {
    int8_t dir = cpu.FetchByte(mem);
    if(cpu.V == 0) {
        cpu.PC += dir;
    }
}
