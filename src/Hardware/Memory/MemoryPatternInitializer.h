#pragma once

#include <cstdint>
#include <span>

namespace Hardware {

class MemoryPatternInitializer {
   public:
    static void Initialize(std::span<uint8_t> memory, bool powerOnPattern = false);
    static void FillZero(std::span<uint8_t> memory);
    static void FillRandom(std::span<uint8_t> memory);
};

}  // namespace Hardware
