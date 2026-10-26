#pragma once

#include <atomic>
#include <chrono>
#include <deque>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include "Hardware/Comm/ESP8266/ATHandler.h"

namespace Hardware {

class NetworkTrafficLogger {
public:
    NetworkTrafficLogger() = default;
    ~NetworkTrafficLogger() = default;

    NetworkTrafficLogger(const NetworkTrafficLogger&) = delete;
    NetworkTrafficLogger& operator=(const NetworkTrafficLogger&) = delete;
    NetworkTrafficLogger(NetworkTrafficLogger&&) = delete;
    NetworkTrafficLogger& operator=(NetworkTrafficLogger&&) = delete;

    inline void Record(TrafficDirection dir, TrafficChannel ch, int linkId, const std::string& tag,
                       const std::string& payload) {
        if (snifferPaused.load(std::memory_order_relaxed)) {
            return;
        }
        uint64_t nowMs = static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch())
                .count());
        TrafficPacket pkt{nowMs, dir, ch, linkId, tag, payload};
        std::lock_guard<std::mutex> lock(trafficMutex);
        trafficLog.push_back(std::move(pkt));
        if (trafficLog.size() > kMaxTrafficPackets) {
            trafficLog.pop_front();
        }
    }

    [[nodiscard]] inline std::vector<TrafficPacket> GetTrafficLog() const {
        std::lock_guard<std::mutex> lock(trafficMutex);
        return {trafficLog.begin(), trafficLog.end()};
    }

    [[nodiscard]] inline std::vector<TrafficPacket> GetLog() const {
        return GetTrafficLog();
    }

    inline void ClearTrafficLog() {
        std::lock_guard<std::mutex> lock(trafficMutex);
        trafficLog.clear();
    }

    inline void Clear() {
        ClearTrafficLog();
    }

    inline void SetSnifferPaused(bool paused) {
        snifferPaused.store(paused, std::memory_order_relaxed);
    }

    inline void SetPaused(bool paused) {
        SetSnifferPaused(paused);
    }

    [[nodiscard]] inline bool IsSnifferPaused() const {
        return snifferPaused.load(std::memory_order_relaxed);
    }

    [[nodiscard]] inline bool IsPaused() const {
        return IsSnifferPaused();
    }

private:
    static constexpr size_t kMaxTrafficPackets = 500;
    std::deque<TrafficPacket> trafficLog;
    mutable std::mutex trafficMutex;
    std::atomic<bool> snifferPaused{false};
};

}  // namespace Hardware
