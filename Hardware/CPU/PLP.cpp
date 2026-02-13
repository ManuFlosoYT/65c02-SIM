#include "PLP.h"

void PLP::Execute(CPU& cpu, Mem& mem) {
    Byte status = cpu.PopByte(mem);
    cpu.SetStatus(status);
}
