#pragma once

namespace Hardware {
class CPU;
class Bus;
}  // namespace Hardware

namespace Hardware::Instructions {

class CLC {
public:
    template <bool Debug>
    static void Execute(CPU& cpu, Bus& bus);
};

class SEC {
public:
    template <bool Debug>
    static void Execute(CPU& cpu, Bus& bus);
};

class CLD {
public:
    template <bool Debug>
    static void Execute(CPU& cpu, Bus& bus);
};

class SED {
public:
    template <bool Debug>
    static void Execute(CPU& cpu, Bus& bus);
};

class CLI {
public:
    template <bool Debug>
    static void Execute(CPU& cpu, Bus& bus);
};

class SEI {
public:
    template <bool Debug>
    static void Execute(CPU& cpu, Bus& bus);
};

class CLV {
public:
    template <bool Debug>
    static void Execute(CPU& cpu, Bus& bus);
};

}  // namespace Hardware::Instructions

#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"

namespace Hardware::Instructions {

template <bool Debug>
inline void CLC::Execute(CPU& cpu, Bus& bus) {
    cpu.C = 0;
}

template <bool Debug>
inline void SEC::Execute(CPU& cpu, Bus& bus) {
    cpu.C = 1;
}

template <bool Debug>
inline void CLD::Execute(CPU& cpu, Bus& bus) {
    cpu.D = 0;
}

template <bool Debug>
inline void SED::Execute(CPU& cpu, Bus& bus) {
    cpu.D = 1;
}

template <bool Debug>
inline void CLI::Execute(CPU& cpu, Bus& bus) {
    cpu.I = 0;
}

template <bool Debug>
inline void SEI::Execute(CPU& cpu, Bus& bus) {
    cpu.I = 1;
}

template <bool Debug>
inline void CLV::Execute(CPU& cpu, Bus& bus) {
    cpu.V = 0;
}

}  // namespace Hardware::Instructions

