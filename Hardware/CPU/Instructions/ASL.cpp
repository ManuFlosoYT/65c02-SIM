#include "ASL.h"

namespace Hardware::Instructions {

static void SetFlags(CPU& cpu, Byte val, Byte A) {
    cpu.Z = (val == 0);
    cpu.N = (val & 0b10000000) > 0;
    cpu.C = (A & 0b10000000) > 0;
}

void ASL::ExecuteAccumulator(CPU& cpu, Bus& bus) {
    
    Byte A = cpu.A;
    cpu.A <<= 1;

    SetFlags(cpu, cpu.A, A);
}

void ASL::ExecuteZP(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte(bus);
    Byte dato = cpu.ReadByte(ZP_Dir, bus);
    Byte A = dato;
    dato <<= 1;
    bus.Write(ZP_Dir, dato);
    SetFlags(cpu, dato, A);
}

void ASL::ExecuteZPX(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte(bus);
    ZP_Dir += cpu.X;
    Byte dato = cpu.ReadByte(ZP_Dir, bus);
    Byte A = dato;
    dato <<= 1;
    bus.Write(ZP_Dir, dato);
    SetFlags(cpu, dato, A);
}

void ASL::ExecuteABS(CPU& cpu, Bus& bus) {
    Word Dir = cpu.FetchWord(bus);
    Byte dato = cpu.ReadByte(Dir, bus);
    Byte A = dato;
    dato <<= 1;
    bus.Write(Dir, dato);
    SetFlags(cpu, dato, A);
}

void ASL::ExecuteABSX(CPU& cpu, Bus& bus) {
    Word Dir = cpu.FetchWord(bus);
    Dir += cpu.X;
    Byte dato = cpu.ReadByte(Dir, bus);
    Byte A = dato;
    dato <<= 1;
    bus.Write(Dir, dato);
    SetFlags(cpu, dato, A);
}

}  // namespace Hardware::Instructions
