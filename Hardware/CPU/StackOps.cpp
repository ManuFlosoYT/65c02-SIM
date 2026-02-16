#include "StackOps.h"

#include "../CPU.h"

namespace Hardware {
namespace CPUStackOps {

void PushByte(CPU& cpu, Byte val, Mem& mem) {
    mem.Write(cpu.SP, val);
    cpu.SP--;
    if (cpu.SP < 0x0100) cpu.SP = 0x01FF;  // Wrap to the top of the stack
}

Byte PopByte(CPU& cpu, Mem& mem) {
    cpu.SP++;
    if (cpu.SP > 0x01FF) cpu.SP = 0x0100;  // Wrap to the bottom of the stack
    return mem.Read(cpu.SP);
}

void PushWord(CPU& cpu, Word val, Mem& mem) {
    PushByte(cpu, (val >> 8) & 0xFF, mem);
    PushByte(cpu, val & 0xFF, mem);
}

Word PopWord(CPU& cpu, Mem& mem) {
    Word Low = PopByte(cpu, mem);
    Word High = PopByte(cpu, mem);
    return (High << 8) | Low;
}

}  // namespace CPUStackOps
}  // namespace Hardware
