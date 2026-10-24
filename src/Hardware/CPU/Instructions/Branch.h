#pragma once

namespace Hardware {
class CPU;
class Bus;
}  // namespace Hardware

namespace Hardware::Instructions {

class BCC {
public:
    template <bool Debug>
    static void Execute(CPU& cpu, Bus& bus);
};

class BCS {
public:
    template <bool Debug>
    static void Execute(CPU& cpu, Bus& bus);
};

class BEQ {
public:
    template <bool Debug>
    static void Execute(CPU& cpu, Bus& bus);
};

class BNE {
public:
    template <bool Debug>
    static void Execute(CPU& cpu, Bus& bus);
};

class BMI {
public:
    template <bool Debug>
    static void Execute(CPU& cpu, Bus& bus);
};

class BPL {
public:
    template <bool Debug>
    static void Execute(CPU& cpu, Bus& bus);
};

class BVC {
public:
    template <bool Debug>
    static void Execute(CPU& cpu, Bus& bus);
};

class BVS {
public:
    template <bool Debug>
    static void Execute(CPU& cpu, Bus& bus);
};

class BRA {
public:
    template <bool Debug>
    static void Execute(CPU& cpu, Bus& bus);
};

}  // namespace Hardware::Instructions

#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"

namespace Hardware::Instructions {

template <bool Debug>
inline void BCC::Execute(CPU& cpu, Bus& bus) {
    int8_t dir = cpu.FetchByte<Debug>(bus);
    if (cpu.C == 0) {
        cpu.remainingCycles++;
        cpu.AddPageCrossPenalty(cpu.PC, static_cast<Word>(cpu.PC + dir));
        cpu.PC += dir;
        cpu.UpdatePagePtr(bus);
    }
}

template <bool Debug>
inline void BCS::Execute(CPU& cpu, Bus& bus) {
    int8_t dir = cpu.FetchByte<Debug>(bus);
    if (cpu.C == 1) {
        cpu.remainingCycles++;
        cpu.AddPageCrossPenalty(cpu.PC, static_cast<Word>(cpu.PC + dir));
        cpu.PC += dir;
        cpu.UpdatePagePtr(bus);
    }
}

template <bool Debug>
inline void BEQ::Execute(CPU& cpu, Bus& bus) {
    int8_t dir = cpu.FetchByte<Debug>(bus);
    if (cpu.Z == 1) {
        cpu.remainingCycles++;
        cpu.AddPageCrossPenalty(cpu.PC, static_cast<Word>(cpu.PC + dir));
        cpu.PC += dir;
        cpu.UpdatePagePtr(bus);
    }
}

template <bool Debug>
inline void BNE::Execute(CPU& cpu, Bus& bus) {
    int8_t dir = cpu.FetchByte<Debug>(bus);
    if (cpu.Z == 0) {
        cpu.remainingCycles++;
        cpu.AddPageCrossPenalty(cpu.PC, static_cast<Word>(cpu.PC + dir));
        cpu.PC += dir;
        cpu.UpdatePagePtr(bus);
    }
}

template <bool Debug>
inline void BMI::Execute(CPU& cpu, Bus& bus) {
    int8_t dir = cpu.FetchByte<Debug>(bus);
    if (cpu.N == 1) {
        cpu.remainingCycles++;
        cpu.AddPageCrossPenalty(cpu.PC, static_cast<Word>(cpu.PC + dir));
        cpu.PC += dir;
        cpu.UpdatePagePtr(bus);
    }
}

template <bool Debug>
inline void BPL::Execute(CPU& cpu, Bus& bus) {
    int8_t dir = cpu.FetchByte<Debug>(bus);
    if (cpu.N == 0) {
        cpu.remainingCycles++;
        cpu.AddPageCrossPenalty(cpu.PC, static_cast<Word>(cpu.PC + dir));
        cpu.PC += dir;
        cpu.UpdatePagePtr(bus);
    }
}

template <bool Debug>
inline void BVC::Execute(CPU& cpu, Bus& bus) {
    int8_t dir = cpu.FetchByte<Debug>(bus);
    if (cpu.V == 0) {
        cpu.remainingCycles++;
        cpu.AddPageCrossPenalty(cpu.PC, static_cast<Word>(cpu.PC + dir));
        cpu.PC += dir;
        cpu.UpdatePagePtr(bus);
    }
}

template <bool Debug>
inline void BVS::Execute(CPU& cpu, Bus& bus) {
    int8_t dir = cpu.FetchByte<Debug>(bus);
    if (cpu.V == 1) {
        cpu.remainingCycles++;
        cpu.AddPageCrossPenalty(cpu.PC, static_cast<Word>(cpu.PC + dir));
        cpu.PC += dir;
        cpu.UpdatePagePtr(bus);
    }
}

template <bool Debug>
inline void BRA::Execute(CPU& cpu, Bus& bus) {
    int8_t offset = cpu.FetchByte<Debug>(bus);
    cpu.AddPageCrossPenalty(cpu.PC, static_cast<Word>(cpu.PC + offset));
    cpu.PC += offset;
    cpu.UpdatePagePtr(bus);
}

}  // namespace Hardware::Instructions

