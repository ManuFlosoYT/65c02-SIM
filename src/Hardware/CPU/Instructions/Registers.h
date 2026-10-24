#pragma once

namespace Hardware {
class CPU;
class Bus;
}  // namespace Hardware

namespace Hardware::Instructions {

class TAX {
public:
    template <bool Debug>
    static void Execute(CPU& cpu, Bus& bus);
};

class TAY {
public:
    template <bool Debug>
    static void Execute(CPU& cpu, Bus& bus);
};

class TXA {
public:
    template <bool Debug>
    static void Execute(CPU& cpu, Bus& bus);
};

class TYA {
public:
    template <bool Debug>
    static void Execute(CPU& cpu, Bus& bus);
};

class TSX {
public:
    template <bool Debug>
    static void Execute(CPU& cpu, Bus& bus);
};

class TXS {
public:
    template <bool Debug>
    static void Execute(CPU& cpu, Bus& bus);
};

class INX {
public:
    template <bool Debug>
    static void Execute(CPU& cpu, Bus& bus);
};

class INY {
public:
    template <bool Debug>
    static void Execute(CPU& cpu, Bus& bus);
};

class DEX {
public:
    template <bool Debug>
    static void Execute(CPU& cpu, Bus& bus);
};

class DEY {
public:
    template <bool Debug>
    static void Execute(CPU& cpu, Bus& bus);
};

}  // namespace Hardware::Instructions

#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"

namespace Hardware::Instructions {

template <bool Debug>
inline void TAX::Execute(CPU& cpu, Bus& bus) {
    cpu.X = cpu.A;
    cpu.Z = (cpu.X == 0);
    cpu.N = (cpu.X & 0b10000000) > 0;
}

template <bool Debug>
inline void TAY::Execute(CPU& cpu, Bus& bus) {
    cpu.Y = cpu.A;
    cpu.Z = (cpu.Y == 0);
    cpu.N = (cpu.Y & 0b10000000) > 0;
}

template <bool Debug>
inline void TXA::Execute(CPU& cpu, Bus& bus) {
    cpu.A = cpu.X;
    cpu.Z = (cpu.A == 0);
    cpu.N = (cpu.A & 0b10000000) > 0;
}

template <bool Debug>
inline void TYA::Execute(CPU& cpu, Bus& bus) {
    cpu.A = cpu.Y;
    cpu.Z = (cpu.A == 0);
    cpu.N = (cpu.A & 0b10000000) > 0;
}

template <bool Debug>
inline void TSX::Execute(CPU& cpu, Bus& bus) {
    cpu.X = cpu.SP;
    cpu.Z = (cpu.X == 0);
    cpu.N = (cpu.X & 0b10000000) > 0;
}

template <bool Debug>
inline void TXS::Execute(CPU& cpu, Bus& bus) {
    cpu.SP = 0x0100 | cpu.X;
}

template <bool Debug>
inline void INX::Execute(CPU& cpu, Bus& bus) {
    cpu.X++;
    cpu.Z = (cpu.X == 0);
    cpu.N = (cpu.X & 0b10000000) > 0;
}

template <bool Debug>
inline void INY::Execute(CPU& cpu, Bus& bus) {
    cpu.Y++;
    cpu.Z = (cpu.Y == 0);
    cpu.N = (cpu.Y & 0b10000000) > 0;
}

template <bool Debug>
inline void DEX::Execute(CPU& cpu, Bus& bus) {
    cpu.X--;
    cpu.Z = (cpu.X == 0);
    cpu.N = (cpu.X & 0b10000000) > 0;
}

template <bool Debug>
inline void DEY::Execute(CPU& cpu, Bus& bus) {
    cpu.Y--;
    cpu.Z = (cpu.Y == 0);
    cpu.N = (cpu.Y & 0b10000000) > 0;
}

}  // namespace Hardware::Instructions

