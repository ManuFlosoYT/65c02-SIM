#pragma once

namespace Hardware {
class CPU;
class Bus;
}  // namespace Hardware

namespace Hardware::Instructions {

class JMP {
public:
    template <bool Debug>
    static void ExecuteABS(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void ExecuteIND(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void ExecuteABSX(CPU& cpu, Bus& bus);
};

class NOP {
public:
    template <bool Debug>
    static void Execute(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void ExecuteImmediate(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void ExecuteZP(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void ExecuteZPX(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void ExecuteABS(CPU& cpu, Bus& bus);
    template <bool Debug>
    static void ExecuteABSX(CPU& cpu, Bus& bus);
};

class WAI {
public:
    template <bool Debug>
    static void Execute(CPU& cpu, Bus& bus);
};

class STP {
public:
    template <bool Debug>
    static void Execute(CPU& cpu, Bus& bus);
};

class JSR {
public:
    template <bool Debug>
    static void Execute(CPU& cpu, Bus& bus);
};

class RTS {
public:
    template <bool Debug>
    static void Execute(CPU& cpu, Bus& bus);
};

class BRK {
public:
    template <bool Debug>
    static void Execute(CPU& cpu, Bus& bus);
};

class RTI {
public:
    template <bool Debug>
    static void Execute(CPU& cpu, Bus& bus);
};

}  // namespace Hardware::Instructions

#include "Hardware/CPU/CPU.h"
#include "Hardware/Core/Bus.h"

namespace Hardware::Instructions {

template <bool Debug>
inline void JMP::ExecuteABS(CPU& cpu, Bus& bus) {
    Word dir = cpu.FetchWord<Debug>(bus);
    cpu.PC = dir;
    cpu.UpdatePagePtr(bus);
}

template <bool Debug>
inline void JMP::ExecuteABSX(CPU& cpu, Bus& bus) {
    Word dir = cpu.FetchWord<Debug>(bus);
    dir += cpu.X;
    Word target = cpu.ReadWord<Debug>(dir, bus);
    cpu.PC = target;
    cpu.UpdatePagePtr(bus);
}

template <bool Debug>
inline void JMP::ExecuteIND(CPU& cpu, Bus& bus) {
    Word dirIND = cpu.FetchWord<Debug>(bus);
    Word dir = cpu.ReadWord<Debug>(dirIND, bus);
    cpu.PC = dir;
    cpu.UpdatePagePtr(bus);
}

template <bool Debug>
inline void NOP::Execute(CPU& cpu, Bus& bus) {}

template <bool Debug>
inline void NOP::ExecuteImmediate(CPU& cpu, Bus& bus) {
    cpu.FetchByte<Debug>(bus);
}

template <bool Debug>
inline void NOP::ExecuteZP(CPU& cpu, Bus& bus) {
    cpu.FetchByte<Debug>(bus);
}

template <bool Debug>
inline void NOP::ExecuteZPX(CPU& cpu, Bus& bus) {
    cpu.FetchByte<Debug>(bus);
}

template <bool Debug>
inline void NOP::ExecuteABS(CPU& cpu, Bus& bus) {
    cpu.FetchWord<Debug>(bus);
}

template <bool Debug>
inline void NOP::ExecuteABSX(CPU& cpu, Bus& bus) {
    cpu.FetchWord<Debug>(bus);
}

template <bool Debug>
inline void WAI::Execute(CPU& cpu, Bus& bus) {
    cpu.waiting = true;
}

template <bool Debug>
inline void STP::Execute(CPU& cpu, Bus& bus) {}

}  // namespace Hardware::Instructions

