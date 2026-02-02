#include "BRA.h"

void BRA::Ejecutar(CPU& cpu, Mem& mem) {
    int8_t offset = cpu.FetchByte(mem);
    cpu.PC += offset;
}
