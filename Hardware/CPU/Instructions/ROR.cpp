#include "ROR.h"

namespace Hardware::Instructions {

static void SetFlags(CPU& cpu, Byte val, Byte A) {
    cpu.Z = (val == 0);
    cpu.N = (val & 0b10000000) > 0;
    cpu.C = (A & 0b00000001) > 0;
}

void ROR::ExecuteAccumulator(CPU& cpu, Bus& bus) {
    Byte A = cpu.A;
    Byte oldCarry = cpu.C ? 0x80 : 0;
    cpu.A = (A >> 1) | oldCarry;

    SetFlags(cpu, cpu.A, A);
}

void ROR::ExecuteZP(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte(bus);
    Byte dato = cpu.ReadByte(ZP_Dir, bus);
    Byte A = dato;
    Byte oldCarry = cpu.C ? 0x80 : 0;
    dato = (dato >> 1) | oldCarry;
    bus.Write(ZP_Dir, dato);
    SetFlags(cpu, dato, A);
}

void ROR::ExecuteZPX(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte(bus);
    ZP_Dir += cpu.X;
    Byte dato = cpu.ReadByte(ZP_Dir, bus);
    Byte A = dato;
    Byte oldCarry = cpu.C ? 0x80 : 0;
    dato = (dato >> 1) | oldCarry;
    bus.Write(ZP_Dir, dato);
    SetFlags(cpu, dato, A);
}

void ROR::ExecuteABS(CPU& cpu, Bus& bus) {
    Word Dir = cpu.FetchWord(bus);
    Byte dato = cpu.ReadByte(Dir, bus);
    Byte A = dato;
    Byte oldCarry = cpu.C ? 0x80 : 0;
    dato = (dato >> 1) | oldCarry;
    bus.Write(Dir, dato);
    SetFlags(cpu, dato, A);
}

void ROR::ExecuteABSX(CPU& cpu, Bus& bus) {
    Word Dir = cpu.FetchWord(bus);
    Dir += cpu.X;
    Byte dato = cpu.ReadByte(Dir, bus);
    Byte A = dato;
    Byte oldCarry = cpu.C ? 0x80 : 0;
    dato = (dato >> 1) | oldCarry;
    bus.Write(Dir, dato);
    SetFlags(cpu, dato, A);
}

}  // namespace Hardware::Instructions
