#include "Hardware/Memory/MemoryPatternInitializer.h"

#include <algorithm>
#include <random>

namespace Hardware {

void MemoryPatternInitializer::FillZero(std::span<uint8_t> memory) {
    std::ranges::fill(memory, 0);
}

void MemoryPatternInitializer::FillRandom(std::span<uint8_t> memory) {
    std::mt19937 rng{std::random_device{}()};
    std::ranges::generate(memory, [&] { return static_cast<uint8_t>(rng()); });
}

void MemoryPatternInitializer::Initialize(std::span<uint8_t> memory, bool powerOnPattern) {
    if (!powerOnPattern) {
        FillZero(memory);
    } else {
        FillRandom(memory);
    }
}

}  // namespace Hardware
