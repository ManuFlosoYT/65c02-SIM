#include "BMI.h"

void BMI::Execute(CPU& cpu, Mem& mem) {
    int8_t dir = cpu.FetchByte(mem);
    if(cpu.N == 1) {
        cpu.PC += dir;
    }
}
