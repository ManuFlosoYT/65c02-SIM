#include "BMI.h"

void BMI::Ejecutar(CPU& cpu, Mem& mem) {
    int8_t dir = cpu.FetchByte(mem);
    if(cpu.N == 1) {
        cpu.PC += dir;
    }
}
