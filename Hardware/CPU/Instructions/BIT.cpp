#include "BIT.h"

namespace Hardware::Instructions {

static void SetFlags(CPU& cpu, Byte val) {
    cpu.N = (val & 0b10000000) > 0;
    cpu.V = (val & 0b01000000) > 0;
}

void BIT::ExecuteImmediate(CPU& cpu, Bus& bus) {
    Byte dato = cpu.FetchByte(bus);
    Byte val = dato & cpu.A;
    cpu.Z = (val == 0);
}

void BIT::ExecuteZP(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte(bus);
    Byte dato = cpu.ReadByte(ZP_Dir, bus);
    Byte val = dato & cpu.A;
    cpu.Z = (val == 0);
    SetFlags(cpu, dato);
}

void BIT::ExecuteZPX(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte(bus);
    ZP_Dir += cpu.X;
    Byte dato = cpu.ReadByte(ZP_Dir, bus);
    Byte val = dato & cpu.A;
    cpu.Z = (val == 0);
    SetFlags(cpu, dato);
}

void BIT::ExecuteABS(CPU& cpu, Bus& bus) {
    Word Dir = cpu.FetchWord(bus);
    Byte dato = cpu.ReadByte(Dir, bus);
    Byte val = dato & cpu.A;
    cpu.Z = (val == 0);
    SetFlags(cpu, dato);
}

void BIT::ExecuteABSX(CPU& cpu, Bus& bus) {
    Word baseAddr = cpu.FetchWord(bus);
    Word effectiveAddr = baseAddr + cpu.X;

    cpu.AddPageCrossPenalty(baseAddr, effectiveAddr);

    Word Dir = effectiveAddr;
    Byte dato = cpu.ReadByte(Dir, bus);
    Byte val = dato & cpu.A;
    cpu.Z = (val == 0);
    SetFlags(cpu, dato);
}

}  // namespace Hardware::Instructions
