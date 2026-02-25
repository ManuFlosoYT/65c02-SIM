#include "BRK.h"

namespace Hardware::Instructions {

void BRK::Execute(CPU& cpu, Bus& bus) {
    cpu.B = true;
    cpu.PushWord(cpu.PC + 1, bus);
    Byte PS = cpu.GetStatus();
    cpu.PushByte(PS, bus);
    cpu.PC = cpu.ReadWord(0xFFFE, bus);
}

}  // namespace Hardware::Instructions
