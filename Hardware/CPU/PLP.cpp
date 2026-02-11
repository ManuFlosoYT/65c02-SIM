#include "PLP.h"

void PLP::Ejecutar(CPU& cpu, Mem& mem) {
    Byte status = cpu.PopByte(mem);
    cpu.SetStatus(status);
}
