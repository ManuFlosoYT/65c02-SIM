#include "DEC.h"

namespace Hardware::Instructions {

static void SetFlags(CPU& cpu, Byte val) {
    cpu.Z = (val == 0);
    cpu.N = (val & 0b10000000) > 0;
}

void DEC::ExecuteAccumulator(CPU& cpu, Mem& mem) {
    cpu.A--;
    SetFlags(cpu, cpu.A);
}

void DEC::ExecuteZP(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);
    Byte dato = cpu.ReadByte(ZP_Dir, mem);
    dato--;
    mem.Write(ZP_Dir, dato);
    SetFlags(cpu, dato);
}

void DEC::ExecuteZPX(CPU& cpu, Mem& mem) {
    Byte ZP_Dir = cpu.FetchByte(mem);
    ZP_Dir += cpu.X;
    Byte dato = cpu.ReadByte(ZP_Dir, mem);
    dato--;
    mem.Write(ZP_Dir, dato);
    SetFlags(cpu, dato);
}

void DEC::ExecuteABS(CPU& cpu, Mem& mem) {
    Word Dir = cpu.FetchWord(mem);
    Byte dato = cpu.ReadByte(Dir, mem);
    dato--;
    mem.Write(Dir, dato);
    SetFlags(cpu, dato);
}

void DEC::ExecuteABSX(CPU& cpu, Mem& mem) {
    Word Dir = cpu.FetchWord(mem);
    Dir += cpu.X;
    Byte dato = cpu.ReadByte(Dir, mem);
    dato--;
    mem.Write(Dir, dato);
    SetFlags(cpu, dato);
}

}  // namespace Hardware::Instructions
