#pragma once

#include <array>
#include <atomic>
#include <cstdint>

#include "Hardware/Core/IBusDevice.h"

namespace Hardware {

constexpr Word MAX_ADDR = 0xFFFF;
constexpr uint32_t BUS_SIZE = static_cast<uint32_t>(MAX_ADDR) + 1;

enum CDLFlags : uint8_t {
    CDL_NONE = 0,
    CDL_OPCODE = 1 << 0,
    CDL_DATA = 1 << 1,
    CDL_ZP_POINTER = 1 << 2,
    CDL_OPERAND = 1 << 3,
};

struct BusTelemetry {
    bool profilingEnabled = false;
    std::array<uint32_t, BUS_SIZE> profilerCounts{};
    std::array<uint32_t, BUS_SIZE> profilerReadCounts{};
    std::array<uint32_t, BUS_SIZE> profilerWriteCounts{};
    std::array<uint8_t, BUS_SIZE> cdlMap{};

    bool heatmapEnabled = false;
    std::atomic<bool> heatmapDirty{false};
    std::array<std::atomic<uint8_t>, BUS_SIZE> readHeat{};
    std::array<std::atomic<uint8_t>, BUS_SIZE> writeHeat{};

    inline void LogRead(Word address) {
        if (profilingEnabled) {
            profilerCounts[address]++;
            profilerReadCounts.at(address)++;
        }
        if (heatmapEnabled) {
            readHeat.at(address).store(255, std::memory_order_relaxed);
            heatmapDirty.store(true, std::memory_order_relaxed);
        }
    }

    inline void LogWrite(Word address) {
        if (profilingEnabled) {
            profilerCounts[address]++;
            profilerWriteCounts.at(address)++;
        }
        if (heatmapEnabled) {
            writeHeat.at(address).store(255, std::memory_order_relaxed);
            heatmapDirty.store(true, std::memory_order_relaxed);
        }
    }

    inline void LogCDL(Word address, uint8_t flags) {
        cdlMap[address] |= flags;
    }

    [[nodiscard]] inline uint8_t GetCDL(Word address) const {
        return cdlMap[address];
    }

    inline void ClearCDL() {
        cdlMap.fill(CDL_NONE);
    }

    [[nodiscard]] inline const std::array<uint8_t, BUS_SIZE>& GetCDLMap() const {
        return cdlMap;
    }

    [[nodiscard]] inline std::array<uint8_t, BUS_SIZE>& GetCDLMap() {
        return cdlMap;
    }

    inline void SetProfilingEnabled(bool enabled) {
        profilingEnabled = enabled;
    }

    [[nodiscard]] inline bool IsProfilingEnabled() const {
        return profilingEnabled;
    }

    inline void ClearProfiler() {
        profilerCounts.fill(0);
        profilerReadCounts.fill(0);
        profilerWriteCounts.fill(0);
    }

    inline uint32_t* GetProfilerCounts() {
        return profilerCounts.data();
    }

    inline uint32_t* GetProfilerReadCounts() {
        return profilerReadCounts.data();
    }

    inline uint32_t* GetProfilerWriteCounts() {
        return profilerWriteCounts.data();
    }

    inline void SetHeatmapEnabled(bool enabled) {
        heatmapEnabled = enabled;
        if (!enabled) {
            ClearHeatmap();
        }
    }

    [[nodiscard]] inline bool IsHeatmapEnabled() const {
        return heatmapEnabled;
    }

    inline void ClearHeatmap() {
        for (auto& r : readHeat) {
            r.store(0, std::memory_order_relaxed);
        }
        for (auto& w : writeHeat) {
            w.store(0, std::memory_order_relaxed);
        }
        heatmapDirty.store(false, std::memory_order_relaxed);
    }

    void DecayHeatmap(uint8_t amount);

    [[nodiscard]] inline uint8_t GetReadHeat(Word address) const {
        return readHeat.at(address).load(std::memory_order_relaxed);
    }

    [[nodiscard]] inline uint8_t GetWriteHeat(Word address) const {
        return writeHeat.at(address).load(std::memory_order_relaxed);
    }

    [[nodiscard]] inline bool HasActiveHooks() const {
        return profilingEnabled || heatmapEnabled;
    }
};

}  // namespace Hardware
