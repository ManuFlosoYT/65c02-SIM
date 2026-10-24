#pragma once

namespace Hardware {
class CPU;
class Bus;
}  // namespace Hardware

namespace Hardware::Instructions {

class PHA {
public:
    template <bool Debug>
    static void Execute(CPU& cpu, Bus& bus);
};

class PLA {
public:
    template <bool Debug>
    static void Execute(CPU& cpu, Bus& bus);
};

class PHX {
public:
    template <bool Debug>
    static void Execute(CPU& cpu, Bus& bus);
};

class PLX {
public:
    template <bool Debug>
    static void Execute(CPU& cpu, Bus& bus);
};

class PHY {
public:
    template <bool Debug>
    static void Execute(CPU& cpu, Bus& bus);
};

class PLY {
public:
    template <bool Debug>
    static void Execute(CPU& cpu, Bus& bus);
};

class PHP {
public:
    template <bool Debug>
    static void Execute(CPU& cpu, Bus& bus);
};

class PLP {
public:
    template <bool Debug>
    static void Execute(CPU& cpu, Bus& bus);
};

}  // namespace Hardware::Instructions

#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"

namespace Hardware::Instructions {

template <bool Debug>
inline void PHA::Execute(CPU& cpu, Bus& bus) {
    cpu.PushByte<Debug>(cpu.A, bus);
}

template <bool Debug>
inline void PLA::Execute(CPU& cpu, Bus& bus) {
    cpu.A = cpu.PopByte<Debug>(bus);
    cpu.Z = (cpu.A == 0);
    cpu.N = (cpu.A & 0b10000000) > 0;
}

template <bool Debug>
inline void PHX::Execute(CPU& cpu, Bus& bus) {
    cpu.PushByte<Debug>(cpu.X, bus);
}

template <bool Debug>
inline void PLX::Execute(CPU& cpu, Bus& bus) {
    cpu.X = cpu.PopByte<Debug>(bus);
    cpu.Z = (cpu.X == 0);
    cpu.N = (cpu.X & 0b10000000) > 0;
}

template <bool Debug>
inline void PHY::Execute(CPU& cpu, Bus& bus) {
    cpu.PushByte<Debug>(cpu.Y, bus);
}

template <bool Debug>
inline void PLY::Execute(CPU& cpu, Bus& bus) {
    cpu.Y = cpu.PopByte<Debug>(bus);
    cpu.Z = (cpu.Y == 0);
    cpu.N = (cpu.Y & 0b10000000) > 0;
}

template <bool Debug>
inline void PHP::Execute(CPU& cpu, Bus& bus) {
    cpu.PushByte<Debug>(cpu.GetStatus() | 0x10, bus);
}

template <bool Debug>
inline void PLP::Execute(CPU& cpu, Bus& bus) {
    Byte status = cpu.PopByte<Debug>(bus);
    cpu.SetStatus(status);
}

}  // namespace Hardware::Instructions

