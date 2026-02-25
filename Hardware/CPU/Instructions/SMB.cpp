#include "SMB.h"

namespace Hardware::Instructions {

void SMB::Execute0(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte(bus);
    Byte data = cpu.ReadByte(ZP_Dir, bus);
    data |= 0b00000001;
    bus.Write(ZP_Dir, data);
}

void SMB::Execute1(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte(bus);
    Byte data = cpu.ReadByte(ZP_Dir, bus);
    data |= 0b00000010;
    bus.Write(ZP_Dir, data);
}

void SMB::Execute2(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte(bus);
    Byte data = cpu.ReadByte(ZP_Dir, bus);
    data |= 0b00000100;
    bus.Write(ZP_Dir, data);
}

void SMB::Execute3(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte(bus);
    Byte data = cpu.ReadByte(ZP_Dir, bus);
    data |= 0b00001000;
    bus.Write(ZP_Dir, data);
}

void SMB::Execute4(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte(bus);
    Byte data = cpu.ReadByte(ZP_Dir, bus);
    data |= 0b00010000;
    bus.Write(ZP_Dir, data);
}

void SMB::Execute5(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte(bus);
    Byte data = cpu.ReadByte(ZP_Dir, bus);
    data |= 0b00100000;
    bus.Write(ZP_Dir, data);
}

void SMB::Execute6(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte(bus);
    Byte data = cpu.ReadByte(ZP_Dir, bus);
    data |= 0b01000000;
    bus.Write(ZP_Dir, data);
}

void SMB::Execute7(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte(bus);
    Byte data = cpu.ReadByte(ZP_Dir, bus);
    data |= 0b10000000;
    bus.Write(ZP_Dir, data);
}

}  // namespace Hardware::Instructions
