#include "BIT.h"

namespace Hardware::Instructions {

static void SetFlags(CPU& cpu, Byte val) {
    cpu.N = (val & 0b10000000) > 0;
    cpu.V = (val & 0b01000000) > 0;
}

void BIT::ExecuteImmediate(CPU& cpu, Mem& mem) {
    Byte dato = cpu.FetchByte(mem);
    Byte val = dato & cpu.A;
    cpu.Z = (val == 0);
}

void BIT::ExecuteZP(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);
    Byte dato = cpu.ReadByte(ZP_Dir, mem);
    Byte val = dato & cpu.A;
    cpu.Z = (val == 0);
    SetFlags(cpu, dato);
}

void BIT::ExecuteZPX(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);
    ZP_Dir += cpu.X;
    Byte dato = cpu.ReadByte(ZP_Dir, mem);
    Byte val = dato & cpu.A;
    cpu.Z = (val == 0);
    SetFlags(cpu, dato);
}

void BIT::ExecuteABS(CPU& cpu, Mem& mem) {
    Word Dir = cpu.FetchWord(mem);
    Byte dato = cpu.ReadByte(Dir, mem);
    Byte val = dato & cpu.A;
    cpu.Z = (val == 0);
    SetFlags(cpu, dato);
}

void BIT::ExecuteABSX(CPU& cpu, Mem& mem) {
    Word baseAddr = cpu.FetchWord(mem);
    Word effectiveAddr = baseAddr + cpu.X;

    cpu.AddPageCrossPenalty(baseAddr, effectiveAddr);

    Word Dir = effectiveAddr;
    Byte dato = cpu.ReadByte(Dir, mem);
    Byte val = dato & cpu.A;
    cpu.Z = (val == 0);
    SetFlags(cpu, dato);
}

}  // namespace Hardware::Instructions
