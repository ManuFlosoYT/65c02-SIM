#pragma once
#include <cstdint>

#include "Hardware/Core/Bus.h"

namespace Hardware {

class CPU;

namespace CPUDispatch {

using InstructionExecutor = void (*)(CPU&, Bus&);

struct OpcodeEntry {
    InstructionExecutor executor;
    uint8_t baseCycles;
    int8_t exitCode;
};

inline int Dispatch(CPU& cpu, Bus& bus);

}  // namespace CPUDispatch

}  // namespace Hardware

#include "Hardware/CPU/Dispatch.inl"
