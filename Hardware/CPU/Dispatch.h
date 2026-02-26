#pragma once
#include <cstdint>

#include "Hardware/Core/Bus.h"

namespace Hardware {

class CPU;

namespace CPUDispatch {

template <bool Debug>
using InstructionExecutor = void (*)(CPU&, Bus&);

template <bool Debug>
struct OpcodeEntry {
    InstructionExecutor<Debug> executor;
    uint8_t baseCycles;
    int8_t exitCode;
};

template <bool Debug>
inline int Dispatch(CPU& cpu, Bus& bus);

}  // namespace CPUDispatch

}  // namespace Hardware

#include "Hardware/CPU/Dispatch.inl"
