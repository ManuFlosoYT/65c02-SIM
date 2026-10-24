#include "Hardware/Core/LogicAnalyzer.h"

namespace Hardware {

void LogicAnalyzer::RecordSample(const LogicSample& sample) {
    if (frozen) {
        return;
    }
    uint64_t idx = writeIndex.fetch_add(1, std::memory_order_relaxed);
    buffer[idx % BUFFER_SIZE] = sample;
}

size_t LogicAnalyzer::GetCount() const {
    uint64_t total = writeIndex.load(std::memory_order_relaxed);
    return total > BUFFER_SIZE ? BUFFER_SIZE : static_cast<size_t>(total);
}

LogicSample LogicAnalyzer::GetSample(size_t reverseOffset) const {
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

LogicSample LogicAnalyzer::GetSampleByIndex(uint64_t absoluteIndex) const {
    return buffer[absoluteIndex % BUFFER_SIZE];
}

}  // namespace Hardware

