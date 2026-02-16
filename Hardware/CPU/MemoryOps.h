#pragma once
#include <cstdint>

#include "../Mem.h"

using Byte = uint8_t;
using Word = uint16_t;

namespace Hardware {

class CPU;

namespace CPUMemoryOps {

const Byte FetchByte(CPU& cpu, const Mem& mem);
const Word FetchWord(CPU& cpu, const Mem& mem);
const Byte ReadByte(const Word addr, Mem& mem);
const Word ReadWord(const Word addr, Mem& mem);

}  // namespace CPUMemoryOps

}  // namespace Hardware
