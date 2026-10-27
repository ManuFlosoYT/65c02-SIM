#pragma once

#include <cstdint>
#include <iosfwd>

#include "Hardware/Audio/ADSREnvelope.h"
#include "Hardware/Audio/ADSRBugMonitor.h"
#include "Hardware/Audio/SIDFilter.h"

namespace Hardware {

struct Oscillator {
    uint32_t accumulator = 0;
    uint32_t frequency = 0;
    uint16_t pulseWidth = 0;
    uint8_t control = 0;

    ADSREnvelope env;
    Oscillator* prevOsc = nullptr;

    uint32_t noiseShift = 0x7FFFF8;
    uint16_t oscOutput = 0;

    ADSRBugMonitor adsrBugMonitor;

    [[nodiscard]] uint64_t GetADSRDelayBugClock() const noexcept { return adsrBugMonitor.GetADSRDelayBugClock(); }

    void Next(SIDModel model);

    bool SaveState(std::ostream& out) const;
    bool LoadState(std::istream& inStream);
};

}  // namespace Hardware
