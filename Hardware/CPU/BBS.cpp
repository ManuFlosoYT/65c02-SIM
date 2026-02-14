#include "BBS.h"    

namespace Hardware::Instructions {

void BBS::Execute0(CPU& cpu, Mem& mem) {
    int8_t offset = cpu.FetchByte(mem);
    Word dir = cpu.PC + offset;

    if ((cpu.A & 0b00000001) != 0) {
        cpu.PC = dir;
    }
}

void BBS::Execute1(CPU& cpu, Mem& mem) {
    int8_t offset = cpu.FetchByte(mem);
    Word dir = cpu.PC + offset;

    if ((cpu.A & 0b00000010) != 0) {
        cpu.PC = dir;
    }
}

void BBS::Execute2(CPU& cpu, Mem& mem) {
    int8_t offset = cpu.FetchByte(mem);
    Word dir = cpu.PC + offset;

    if ((cpu.A & 0b00000100) != 0) {
        cpu.PC = dir;
    }
}

void BBS::Execute3(CPU& cpu, Mem& mem) {
    int8_t offset = cpu.FetchByte(mem);
    Word dir = cpu.PC + offset;

    if ((cpu.A & 0b00001000) != 0) {
        cpu.PC = dir;
    }
}

void BBS::Execute4(CPU& cpu, Mem& mem) {
    int8_t offset = cpu.FetchByte(mem);
    Word dir = cpu.PC + offset;

    if ((cpu.A & 0b00010000) != 0) {
        cpu.PC = dir;
    }
}

void BBS::Execute5(CPU& cpu, Mem& mem) {
    int8_t offset = cpu.FetchByte(mem);
    Word dir = cpu.PC + offset;

    if ((cpu.A & 0b00100000) != 0) {
        cpu.PC = dir;
    }
}

void BBS::Execute6(CPU& cpu, Mem& mem) {
    int8_t offset = cpu.FetchByte(mem);
    Word dir = cpu.PC + offset;

    if ((cpu.A & 0b01000000) != 0) {
        cpu.PC = dir;
    }
}

void BBS::Execute7(CPU& cpu, Mem& mem) {
    int8_t offset = cpu.FetchByte(mem);
    Word dir = cpu.PC + offset;

    if ((cpu.A & 0b10000000) != 0) {
        cpu.PC = dir;
    }
}

}  // namespace Hardware::Instructions
