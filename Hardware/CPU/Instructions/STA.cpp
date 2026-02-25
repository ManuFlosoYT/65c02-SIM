#include "STA.h"

namespace Hardware::Instructions {

void STA::ExecuteZP(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte(bus);
    bus.Write(ZP_Dir, cpu.A);
}

void STA::ExecuteZPX(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte(bus);
    ZP_Dir += cpu.X;
    bus.Write(ZP_Dir, cpu.A);
}

void STA::ExecuteABS(CPU& cpu, Bus& bus) {
    Word Dir = cpu.FetchWord(bus);
    bus.Write(Dir, cpu.A);
}

void STA::ExecuteABSX(CPU& cpu, Bus& bus) {
    Word Dir = cpu.FetchWord(bus);
    Dir += cpu.X;
    bus.Write(Dir, cpu.A);
}

void STA::ExecuteABSY(CPU& cpu, Bus& bus) {
    Word Dir = cpu.FetchWord(bus);
    Dir += cpu.Y;
    bus.Write(Dir, cpu.A);
}

void STA::ExecuteINDX(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte(bus);
    ZP_Dir += cpu.X;
    Word Dir = cpu.ReadWord(ZP_Dir, bus);
    bus.Write(Dir, cpu.A);
}

void STA::ExecuteINDY(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte(bus);
    Word dir;

    if (ZP_Dir != 0xFF) {
        dir = cpu.ReadWord(ZP_Dir, bus);
    } else {
        Byte low = cpu.ReadByte(0xFF, bus);
        Byte high = cpu.ReadByte(0x00, bus);
        dir = (high << 8) | low;
    }

    dir += cpu.Y;

    bus.Write(dir, cpu.A);
}


void STA::ExecuteINDZP( CPU& cpu, Bus& bus){
Byte ZP_Dir = cpu.FetchByte(bus);
    Word dir;

    if (ZP_Dir != 0xFF) {
        dir = cpu.ReadWord(ZP_Dir, bus);
    } else {
        Byte low = cpu.ReadByte(0xFF, bus);
        Byte high = cpu.ReadByte(0x00, bus);
        dir = (high << 8) | low;
    }

    bus.Write(dir, cpu.A);
}

}  // namespace Hardware::Instructions
