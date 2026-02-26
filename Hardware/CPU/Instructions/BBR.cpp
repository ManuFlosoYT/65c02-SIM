#include "BBR.h"
#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"
#include "InstructionSet.h"

namespace Hardware::Instructions {

void BBR::Execute0(CPU& cpu, Bus& bus) {
    int8_t offset = cpu.FetchByte(bus);
    Word dir = cpu.PC + offset;

    if ((cpu.A & 0b00000001) == 0) {
        cpu.PC = dir;
        cpu.UpdatePagePtr(bus);
    }
}

void BBR::Execute1(CPU& cpu, Bus& bus) {
    int8_t offset = cpu.FetchByte(bus);
    Word dir = cpu.PC + offset;

    if ((cpu.A & 0b00000010) == 0) {
        cpu.PC = dir;
        cpu.UpdatePagePtr(bus);
    }
}

void BBR::Execute2(CPU& cpu, Bus& bus) {
    int8_t offset = cpu.FetchByte(bus);
    Word dir = cpu.PC + offset;

    if ((cpu.A & 0b00000100) == 0) {
        cpu.PC = dir;
        cpu.UpdatePagePtr(bus);
    }
}

void BBR::Execute3(CPU& cpu, Bus& bus) {
    int8_t offset = cpu.FetchByte(bus);
    Word dir = cpu.PC + offset;

    if ((cpu.A & 0b00001000) == 0) {
        cpu.PC = dir;
        cpu.UpdatePagePtr(bus);
    }
}

void BBR::Execute4(CPU& cpu, Bus& bus) {
    int8_t offset = cpu.FetchByte(bus);
    Word dir = cpu.PC + offset;

    if ((cpu.A & 0b00010000) == 0) {
        cpu.PC = dir;
        cpu.UpdatePagePtr(bus);
    }
}

void BBR::Execute5(CPU& cpu, Bus& bus) {
    int8_t offset = cpu.FetchByte(bus);
    Word dir = cpu.PC + offset;

    if ((cpu.A & 0b00100000) == 0) {
        cpu.PC = dir;
        cpu.UpdatePagePtr(bus);
    }
}

void BBR::Execute6(CPU& cpu, Bus& bus) {
    int8_t offset = cpu.FetchByte(bus);
    Word dir = cpu.PC + offset;

    if ((cpu.A & 0b01000000) == 0) {
        cpu.PC = dir;
        cpu.UpdatePagePtr(bus);
    }
}

void BBR::Execute7(CPU& cpu, Bus& bus) {
    int8_t offset = cpu.FetchByte(bus);
    Word dir = cpu.PC + offset;

    if ((cpu.A & 0b10000000) == 0) {
        cpu.PC = dir;
        cpu.UpdatePagePtr(bus);
    }
}

}  // namespace Hardware::Instructions
