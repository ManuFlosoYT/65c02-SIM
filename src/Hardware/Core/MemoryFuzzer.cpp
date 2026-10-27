#include "Hardware/Core/MemoryFuzzer.h"

#include <algorithm>
#include <random>

namespace Hardware {

void MemoryFuzzer::FuzzMemory(Bus& bus, uint16_t startAddr, uint16_t endAddr) {
    if (startAddr > endAddr) {
        std::swap(startAddr, endAddr);
    }
    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> dist(0, 255);
    for (int i = startAddr; i <= static_cast<int>(endAddr); ++i) {
        bus.WriteDirect(static_cast<Word>(i), static_cast<Byte>(dist(rng)));
    }
}

void MemoryFuzzer::CorruptRandomByte(Bus& bus, uint16_t startAddr, uint16_t endAddr) {
    if (startAddr > endAddr) {
        std::swap(startAddr, endAddr);
    }
    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> addrDist(startAddr, endAddr);
    std::uniform_int_distribution<int> valDist(0, 0xFF);

    Word addr = static_cast<Word>(addrDist(rng));
    Byte val = static_cast<Byte>(valDist(rng));
    bus.WriteDirect(addr, val);
}

}  // namespace Hardware
