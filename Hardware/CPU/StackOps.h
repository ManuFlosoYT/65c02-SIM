#pragma once
#include <cstdint>

#include "../Mem.h"

using Byte = uint8_t;
using Word = uint16_t;

namespace Hardware {

class CPU;

namespace CPUStackOps {

void PushByte(CPU& cpu, Byte val, Mem& mem);
Byte PopByte(CPU& cpu, Mem& mem);
void PushWord(CPU& cpu, Word val, Mem& mem);
Word PopWord(CPU& cpu, Mem& mem);

}  // namespace CPUStackOps

}  // namespace Hardware
