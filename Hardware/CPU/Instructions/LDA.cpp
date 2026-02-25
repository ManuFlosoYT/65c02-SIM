#include "LDA.h"
#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"
#include "InstructionSet.h"

namespace Hardware::Instructions {

static void SetFlags(CPU& cpu) {
    cpu.Z = (cpu.A == 0);
    cpu.N = (cpu.A & 0b10000000) > 0;
}

void LDA::ExecuteImmediate(CPU& cpu, Bus& bus) {
    Byte dato = cpu.FetchByte(bus);
    cpu.A = dato;

    SetFlags(cpu);
}

void LDA::ExecuteZP(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte(bus);

    cpu.A = cpu.ReadByte(ZP_Dir, bus);
    SetFlags(cpu);
}

void LDA::ExecuteZPX(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte(bus);
    ZP_Dir += cpu.X;

    cpu.A = cpu.ReadByte(ZP_Dir, bus);
    SetFlags(cpu);
}

void LDA::ExecuteABS(CPU& cpu, Bus& bus) {
    Word Dir = cpu.FetchWord(bus);

    cpu.A = cpu.ReadByte(Dir, bus);
    SetFlags(cpu);
}

void LDA::ExecuteABSX(CPU& cpu, Bus& bus) {
    Word baseAddr = cpu.FetchWord(bus);
    Word effectiveAddr = baseAddr + cpu.X;

    cpu.AddPageCrossPenalty(baseAddr, effectiveAddr);

    cpu.A = cpu.ReadByte(effectiveAddr, bus);
    SetFlags(cpu);
}

void LDA::ExecuteABSY(CPU& cpu, Bus& bus) {
    Word baseAddr = cpu.FetchWord(bus);
    Word effectiveAddr = baseAddr + cpu.Y;

    cpu.AddPageCrossPenalty(baseAddr, effectiveAddr);

    cpu.A = cpu.ReadByte(effectiveAddr, bus);
    SetFlags(cpu);
}

void LDA::ExecuteINDX(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte(bus);
    ZP_Dir += cpu.X;

    Word Dir = cpu.ReadWord(ZP_Dir, bus);

    cpu.A = cpu.ReadByte(Dir, bus);
    SetFlags(cpu);
}

void LDA::ExecuteINDY(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte(bus);

    Word baseAddr;

    if (ZP_Dir != 0xFF) {
        baseAddr = cpu.ReadWord(ZP_Dir, bus);
    } else {
        Byte low = cpu.ReadByte(0xFF, bus);
        Byte high = cpu.ReadByte(0x00, bus);
        baseAddr = (high << 8) | low;
    }

    Word effectiveAddr = baseAddr + cpu.Y;

    cpu.AddPageCrossPenalty(baseAddr, effectiveAddr);

    cpu.A = cpu.ReadByte(effectiveAddr, bus);
    SetFlags(cpu);
}

void LDA::ExecuteIND_ZP(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte(bus);

    Word dir;

    if (ZP_Dir != 0xFF) {
        dir = cpu.ReadWord(ZP_Dir, bus);
    } else {
        Byte low = cpu.ReadByte(0xFF, bus);
        Byte high = cpu.ReadByte(0x00, bus);
        dir = (high << 8) | low;
    }

    cpu.A = cpu.ReadByte(dir, bus);
    SetFlags(cpu);
}

}  // namespace Hardware::Instructions
