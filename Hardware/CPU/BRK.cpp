#include "BRK.h"

void BRK::Ejecutar(CPU& cpu, Mem& mem) {
    cpu.B = true;
    cpu.PushWord(cpu.PC + 1, mem);
    Byte PS = cpu.GetStatus();
    cpu.PushByte(PS, mem);
    cpu.PC = cpu.LeerWord(0xFFFE, mem);
}
