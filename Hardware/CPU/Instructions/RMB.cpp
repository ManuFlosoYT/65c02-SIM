#include "RMB.h"
#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"
#include "InstructionSet.h"

namespace Hardware::Instructions {

void RMB::Execute0(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte(bus);
    Byte data = cpu.ReadByte(ZP_Dir, bus);
    data &= 0b11111110;
    bus.Write(ZP_Dir, data);
}

void RMB::Execute1(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte(bus);
    Byte data = cpu.ReadByte(ZP_Dir, bus);
    data &= 0b11111101;
    bus.Write(ZP_Dir, data);
}

void RMB::Execute2(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte(bus);
    Byte data = cpu.ReadByte(ZP_Dir, bus);
    data &= 0b11111011;
    bus.Write(ZP_Dir, data);
}

void RMB::Execute3(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte(bus);
    Byte data = cpu.ReadByte(ZP_Dir, bus);
    data &= 0b11110111;
    bus.Write(ZP_Dir, data);
}

void RMB::Execute4(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte(bus);
    Byte data = cpu.ReadByte(ZP_Dir, bus);
    data &= 0b11101111;
    bus.Write(ZP_Dir, data);
}

void RMB::Execute5(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte(bus);
    Byte data = cpu.ReadByte(ZP_Dir, bus);
    data &= 0b11011111;
    bus.Write(ZP_Dir, data);
}

void RMB::Execute6(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte(bus);
    Byte data = cpu.ReadByte(ZP_Dir, bus);
    data &= 0b10111111;
    bus.Write(ZP_Dir, data);
}

void RMB::Execute7(CPU& cpu, Bus& bus) {
    Byte ZP_Dir = cpu.FetchByte(bus);
    Byte data = cpu.ReadByte(ZP_Dir, bus);
    data &= 0b01111111;
    bus.Write(ZP_Dir, data);
}

}  // namespace Hardware::Instructions
