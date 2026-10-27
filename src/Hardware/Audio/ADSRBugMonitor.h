#pragma once

#include <atomic>
#include <cstdint>

namespace Hardware {

struct ADSRBugMonitor {
    uint64_t lastGateOffClock = 0;
    bool gateWasActive = false;
    bool hadHardRestart = false;
    std::atomic<uint64_t> adsrDelayBugClock{0};

    constexpr void Reset() noexcept {
        lastGateOffClock = 0;
        gateWasActive = false;
        hadHardRestart = false;
        if !consteval {
            adsrDelayBugClock.store(0, std::memory_order_relaxed);
        }
    }

    [[nodiscard]] uint64_t GetADSRDelayBugClock() const noexcept {
        return adsrDelayBugClock.load(std::memory_order_relaxed);
    }

    void OnGateOff(uint64_t clockCounter, bool hardRestart) noexcept {
        gateWasActive = true;
        lastGateOffClock = clockCounter;
        hadHardRestart = hardRestart;
    }

    void OnGateOn(uint64_t clockCounter, uint64_t twoFrames, bool isAttackZero) noexcept {
        if (gateWasActive && lastGateOffClock > 0) {
            const uint64_t elapsed = clockCounter - lastGateOffClock;
            if ((elapsed < twoFrames && !hadHardRestart) || isAttackZero) {
                adsrDelayBugClock.store(clockCounter, std::memory_order_relaxed);
            }
        }
        gateWasActive = true;
        hadHardRestart = false;
    }

    void MarkHardRestart() noexcept { hadHardRestart = true; }
};

}  // namespace Hardware
