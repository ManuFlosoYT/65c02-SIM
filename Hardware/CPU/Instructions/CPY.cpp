#include "CPY.h"

namespace Hardware::Instructions {

static void SetFlags(CPU& cpu, Byte dato) {
    cpu.C = (cpu.Y >= dato);
    cpu.Z = (cpu.Y == dato);
    cpu.N = ((cpu.Y - dato) & 0b10000000) > 0;
}

void CPY::ExecuteImmediate(CPU& cpu, Mem& mem) {
    Byte dato = cpu.FetchByte(mem);

    SetFlags(cpu, dato);
}

void CPY::ExecuteZP(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);

    Byte dato = cpu.ReadByte(ZP_Dir, mem);
    SetFlags(cpu, dato);
}

void CPY::ExecuteABS(CPU& cpu, Mem& mem) {
    Word Dir = cpu.FetchWord(mem);

    Byte dato = cpu.ReadByte(Dir, mem);
    SetFlags(cpu, dato);
}

}  // namespace Hardware::Instructions
