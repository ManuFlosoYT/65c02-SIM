#include "MemoryOps.h"

#include "../CPU.h"

namespace Hardware {
namespace CPUMemoryOps {

const Byte FetchByte(CPU& cpu, const Mem& mem) {
    Byte dato = mem[cpu.PC];
    cpu.PC++;
    return dato;
}

const Word FetchWord(CPU& cpu, const Mem& mem) {
    Word dato = mem[cpu.PC];
    dato |= (mem[cpu.PC + 1] << 8);
    cpu.PC += 2;
    return dato;
}

const Byte ReadByte(const Word addr, Mem& mem) { 
    return mem.Read(addr); 
}

const Word ReadWord(const Word addr, Mem& mem) {
    Word dato = mem.Read(addr);
    dato |= (mem.Read(addr + 1) << 8);
    return dato;
}

}  // namespace CPUMemoryOps
}  // namespace Hardware
