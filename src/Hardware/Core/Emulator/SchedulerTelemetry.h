#pragma once

#include <atomic>
#include <cstdint>

namespace Core {

struct RuntimeStats {
    uint64_t cyclesPerSecond = 0;
    uint64_t cpuCyclesPerSecond = 0;
    uint64_t gpuCyclesPerSecond = 0;
    uint64_t instructionsPerSecond = 0;
    uint64_t slicesPerSecond = 0;
    uint64_t emulationNanoseconds = 0;
    uint64_t sleepNanoseconds = 0;
    uint64_t lateNanoseconds = 0;
    uint64_t avgBatchDurationNs = 0;
    uint64_t lastBatchDurationNs = 0;
    uint64_t avgBatchCycles = 0;
    uint64_t lastBatchCycles = 0;
    uint64_t batchesPerSecond = 0;
};

struct SchedulerTelemetry {
    std::atomic<uint64_t> actualCyclesPerSecond{0};
    std::atomic<uint64_t> actualCpuCyclesPerSecond{0};
    std::atomic<uint64_t> actualGpuCyclesPerSecond{0};
    std::atomic<uint64_t> actualInstructionsPerSecond{0};
    std::atomic<uint64_t> slicesPerSecond{0};
    std::atomic<uint64_t> emulationNanoseconds{0};
    std::atomic<uint64_t> sleepNanoseconds{0};
    std::atomic<uint64_t> lateNanoseconds{0};
    std::atomic<uint64_t> avgBatchDurationNs{0};
    std::atomic<uint64_t> lastBatchDurationNs{0};
    std::atomic<uint64_t> avgBatchCycles{0};
    std::atomic<uint64_t> lastBatchCycles{0};
    std::atomic<uint64_t> batchesPerSecond{0};

    uint64_t batchDurationThisSecond{0};
    uint64_t batchCyclesThisSecond{0};
    uint64_t batchesThisSecond{0};
    uint64_t currentLastBatchDurationNs{0};
    uint64_t currentLastBatchCycles{0};

    [[nodiscard]] RuntimeStats Snapshot() const {
        return {
            .cyclesPerSecond = actualCyclesPerSecond.load(std::memory_order_relaxed),
            .cpuCyclesPerSecond = actualCpuCyclesPerSecond.load(std::memory_order_relaxed),
            .gpuCyclesPerSecond = actualGpuCyclesPerSecond.load(std::memory_order_relaxed),
            .instructionsPerSecond = actualInstructionsPerSecond.load(std::memory_order_relaxed),
            .slicesPerSecond = slicesPerSecond.load(std::memory_order_relaxed),
            .emulationNanoseconds = emulationNanoseconds.load(std::memory_order_relaxed),
            .sleepNanoseconds = sleepNanoseconds.load(std::memory_order_relaxed),
            .lateNanoseconds = lateNanoseconds.load(std::memory_order_relaxed),
            .avgBatchDurationNs = avgBatchDurationNs.load(std::memory_order_relaxed),
            .lastBatchDurationNs = lastBatchDurationNs.load(std::memory_order_relaxed),
            .avgBatchCycles = avgBatchCycles.load(std::memory_order_relaxed),
            .lastBatchCycles = lastBatchCycles.load(std::memory_order_relaxed),
            .batchesPerSecond = batchesPerSecond.load(std::memory_order_relaxed),
        };
    }

    void CommitSecond(uint64_t cycles, uint64_t cpuCycles, uint64_t gpuCycles,
                      uint64_t instructions, uint64_t slices, uint64_t emulationNs,
                      uint64_t sleepNs, uint64_t lateNs) {
        actualCyclesPerSecond.store(cycles, std::memory_order_relaxed);
        actualCpuCyclesPerSecond.store(cpuCycles, std::memory_order_relaxed);
        actualGpuCyclesPerSecond.store(gpuCycles, std::memory_order_relaxed);
        actualInstructionsPerSecond.store(instructions, std::memory_order_relaxed);
        slicesPerSecond.store(slices, std::memory_order_relaxed);
        emulationNanoseconds.store(emulationNs, std::memory_order_relaxed);
        sleepNanoseconds.store(sleepNs, std::memory_order_relaxed);
        lateNanoseconds.store(lateNs, std::memory_order_relaxed);
        avgBatchDurationNs.store(batchesThisSecond > 0 ? (batchDurationThisSecond / batchesThisSecond) : 0, std::memory_order_relaxed);
        avgBatchCycles.store(batchesThisSecond > 0 ? (batchCyclesThisSecond / batchesThisSecond) : 0, std::memory_order_relaxed);
        lastBatchDurationNs.store(currentLastBatchDurationNs, std::memory_order_relaxed);
        lastBatchCycles.store(currentLastBatchCycles, std::memory_order_relaxed);
        batchesPerSecond.store(batchesThisSecond, std::memory_order_relaxed);

        batchDurationThisSecond = 0;
        batchCyclesThisSecond = 0;
        batchesThisSecond = 0;
    }

    void RecordBatch(uint64_t durationNs, uint64_t cycles) {
        batchDurationThisSecond += durationNs;
        batchCyclesThisSecond += cycles;
        batchesThisSecond++;
        currentLastBatchDurationNs = durationNs;
        currentLastBatchCycles = cycles;
    }
};

}  // namespace Core
