#include "RTI.h"

void RTI::Ejecutar(CPU& cpu, Mem& mem) {
    Byte status = cpu.PopByte(mem);
    cpu.SetStatus(status);
    cpu.PC = cpu.PopWord(mem);
}
