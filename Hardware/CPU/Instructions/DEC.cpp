#include "DEC.h"

namespace Hardware::Instructions {

static void SetFlags(CPU& cpu, Byte val) {
    cpu.Z = (val == 0);
    cpu.N = (val & 0b10000000) > 0;
}

void DEC::ExecuteAccumulator(CPU& cpu, Bus& bus) {
    cpu.A--;
    SetFlags(cpu, cpu.A);
}

void DEC::ExecuteZP(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte(bus);
    Byte dato = cpu.ReadByte(ZP_Dir, bus);
    dato--;
    bus.Write(ZP_Dir, dato);
    SetFlags(cpu, dato);
}

void DEC::ExecuteZPX(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte(bus);
    ZP_Dir += cpu.X;
    Byte dato = cpu.ReadByte(ZP_Dir, bus);
    dato--;
    bus.Write(ZP_Dir, dato);
    SetFlags(cpu, dato);
}

void DEC::ExecuteABS(CPU& cpu, Bus& bus) {
    Word Dir = cpu.FetchWord(bus);
    Byte dato = cpu.ReadByte(Dir, bus);
    dato--;
    bus.Write(Dir, dato);
    SetFlags(cpu, dato);
}

void DEC::ExecuteABSX(CPU& cpu, Bus& bus) {
    Word Dir = cpu.FetchWord(bus);
    Dir += cpu.X;
    Byte dato = cpu.ReadByte(Dir, bus);
    dato--;
    bus.Write(Dir, dato);
    SetFlags(cpu, dato);
}

}  // namespace Hardware::Instructions
