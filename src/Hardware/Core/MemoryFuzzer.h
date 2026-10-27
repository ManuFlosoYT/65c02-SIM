#pragma once

#include <cstdint>

#include "Hardware/Core/Bus.h"

namespace Hardware {

class MemoryFuzzer {
public:
    static void FuzzMemory(Bus& bus, uint16_t startAddr, uint16_t endAddr);
    static void CorruptRandomByte(Bus& bus, uint16_t startAddr = 0x0000, uint16_t endAddr = 0xFFFF);
};

}  // namespace Hardware
