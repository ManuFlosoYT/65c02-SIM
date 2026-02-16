#pragma once
#include <cstdint>

using Byte = uint8_t;

namespace Hardware {

class CPU;

namespace CPUStatusOps {

const Byte GetStatus(const CPU& cpu);
void SetStatus(CPU& cpu, Byte status);

}  // namespace CPUStatusOps

}  // namespace Hardware
