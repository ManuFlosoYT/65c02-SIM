#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>

namespace Hardware {

struct LogicSample {
    uint64_t cycle = 0;
    uint16_t address = 0;
    uint8_t data = 0;
    uint8_t rw : 1 = 1;
    uint8_t sync : 1 = 0;
    uint8_t rdy : 1 = 1;
    uint8_t irq : 1 = 0;
    uint8_t nmi : 1 = 0;
    uint8_t ca1 : 1 = 1;
    uint8_t ca2 : 1 = 1;
    uint8_t cb1 : 1 = 1;
    uint8_t cb2 : 1 = 1;
    uint8_t spiCs : 1 = 1;
    uint8_t spiClk : 1 = 0;
    uint8_t spiMosi : 1 = 0;
    uint8_t spiMiso : 1 = 0;
    uint8_t uartTx : 1 = 1;
    uint8_t uartRx : 1 = 1;
    uint8_t portA = 0;
    uint8_t portB = 0;
};

class LogicAnalyzer {
   public:
    static constexpr size_t BUFFER_SIZE = 4096;

    LogicAnalyzer() = default;

    void Reset() {
        writeIndex.store(0, std::memory_order_relaxed);
        frozen = false;
    }

    void RecordSample(const LogicSample& sample) {
        if (frozen) {
            return;
        }
        uint64_t idx = writeIndex.fetch_add(1, std::memory_order_relaxed);
        buffer[idx % BUFFER_SIZE] = sample;
    }

    [[nodiscard]] uint64_t GetTotalSamples() const { return writeIndex.load(std::memory_order_relaxed); }

    [[nodiscard]] size_t GetCount() const {
        uint64_t total = writeIndex.load(std::memory_order_relaxed);
        return total > BUFFER_SIZE ? BUFFER_SIZE : static_cast<size_t>(total);
    }

    [[nodiscard]] LogicSample GetSample(size_t reverseOffset) const {
        uint64_t total = writeIndex.load(std::memory_order_relaxed);
        if (total == 0) {
            return {};
        }
        if (reverseOffset >= BUFFER_SIZE || reverseOffset >= total) {
            reverseOffset = (total > BUFFER_SIZE ? BUFFER_SIZE - 1 : static_cast<size_t>(total - 1));
        }
        uint64_t target = total - 1 - reverseOffset;
        return buffer[target % BUFFER_SIZE];
    }

    [[nodiscard]] LogicSample GetSampleByIndex(uint64_t absoluteIndex) const {
        return buffer[absoluteIndex % BUFFER_SIZE];
    }

    [[nodiscard]] bool IsFrozen() const { return frozen; }
    void SetFrozen(bool freeze) { frozen = freeze; }
    void Clear() { writeIndex.store(0, std::memory_order_relaxed); }

   private:
    std::array<LogicSample, BUFFER_SIZE> buffer{};
    std::atomic<uint64_t> writeIndex{0};
    bool frozen = false;
};

}  // namespace Hardware

