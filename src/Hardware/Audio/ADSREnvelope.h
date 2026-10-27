#pragma once

#include <array>
#include <cstdint>
#include <iosfwd>

namespace Hardware {

inline constexpr std::array<uint16_t, 16> RATE_COUNTER_PERIODS = {
      9,     32,     63,     95,    149,    220,    267,    313,
    392,    977,   1954,   3126,   3907,  11720,  19532,  31251
};

inline constexpr std::array<uint8_t, 16> SUSTAIN_LEVELS = {
  0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
  0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff
};

struct ADSREnvelope {
    enum State : std::uint8_t { IDLE, ATTACK, DECAY, SUSTAIN, RELEASE };
    State state = IDLE;
    uint8_t level = 0;

    uint16_t rateCounter = 0;
    uint16_t ratePeriod = 0;
    uint8_t exponentialCounter = 0;
    uint8_t exponentialCounterPeriod = 1;
    bool holdZero = true;
    bool gate = false;

    uint8_t attackRate = 0;
    uint8_t decayRate = 0;
    uint8_t sustainLevel = 0;
    uint8_t releaseRate = 0;

    uint8_t targetAttackRate = 0;
    uint8_t targetDecayRate = 0;
    uint8_t targetSustainLevel = 0;
    uint8_t targetReleaseRate = 0;

    void Update(bool gate);

    bool SaveState(std::ostream& out) const;
    bool LoadState(std::istream& inStream);
};

}  // namespace Hardware
