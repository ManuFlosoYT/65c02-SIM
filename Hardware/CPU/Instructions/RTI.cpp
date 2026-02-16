#include "RTI.h"

namespace Hardware::Instructions {

void RTI::Execute(CPU& cpu, Mem& mem) {
    Byte status = cpu.PopByte(mem);
    cpu.SetStatus(status);
    cpu.PC = cpu.PopWord(mem);
}

}  // namespace Hardware::Instructions
