#include "Hardware/Audio/ADSREnvelope.h"
#include "Hardware/Core/ISerializable.h"

#include <iostream>

namespace Hardware {

void ADSREnvelope::Update(bool newGate) {
    if (gate != newGate) [[unlikely]] {
        bool gateRising = (!gate && newGate);
        gate = newGate;
        if (gateRising) {
            state = ATTACK;
            attackRate = targetAttackRate;
            decayRate = targetDecayRate;
            sustainLevel = targetSustainLevel;
            releaseRate = targetReleaseRate;
            ratePeriod = RATE_COUNTER_PERIODS.at(attackRate);
            holdZero = false;
        } else {
            state = RELEASE;
            attackRate = targetAttackRate;
            decayRate = targetDecayRate;
            sustainLevel = targetSustainLevel;
            releaseRate = targetReleaseRate;
            ratePeriod = RATE_COUNTER_PERIODS.at(releaseRate);
        }
    }

    if ((++rateCounter & 0x8000) != 0) [[unlikely]] {
        ++rateCounter &= 0x7FFF;
    }

    if (rateCounter != ratePeriod) [[likely]] {
        return;
    }

    rateCounter = 0;

    attackRate = targetAttackRate;
    decayRate = targetDecayRate;
    sustainLevel = targetSustainLevel;
    releaseRate = targetReleaseRate;

    if (state == ATTACK) ratePeriod = RATE_COUNTER_PERIODS[attackRate];
    else if (state == DECAY) ratePeriod = RATE_COUNTER_PERIODS[decayRate];
    else if (state == RELEASE) ratePeriod = RATE_COUNTER_PERIODS[releaseRate];

    if (state == ATTACK || ++exponentialCounter == exponentialCounterPeriod) {
        exponentialCounter = 0;

        if (holdZero) return;

        switch (state) {
            case ATTACK:
                ++level &= 0xFF;
                if (level == 0xFF) {
                    state = DECAY;
                    ratePeriod = RATE_COUNTER_PERIODS[decayRate];
                }
                break;
            case DECAY:
                if (level == SUSTAIN_LEVELS[sustainLevel]) {
                    state = SUSTAIN;
                    break;
                }
                --level;
                break;
            case SUSTAIN:
                if (level == SUSTAIN_LEVELS[sustainLevel]) {
                    break;
                }
                --level;
                break;
            case RELEASE:
                --level &= 0xFF;
                break;
            default:
                break;
        }

        switch (level) {
            case 0xFF: exponentialCounterPeriod = 1; break;
            case 0x5D: exponentialCounterPeriod = 2; break;
            case 0x36: exponentialCounterPeriod = 4; break;
            case 0x1A: exponentialCounterPeriod = 8; break;
            case 0x0E: exponentialCounterPeriod = 16; break;
            case 0x06: exponentialCounterPeriod = 30; break;
            case 0x00:
                exponentialCounterPeriod = 1;
                holdZero = true;
                break;
        }
    }
}

bool ADSREnvelope::SaveState(std::ostream& out) const {
    Hardware::ISerializable::Serialize(out, state);
    Hardware::ISerializable::Serialize(out, level);
    Hardware::ISerializable::Serialize(out, rateCounter);
    Hardware::ISerializable::Serialize(out, ratePeriod);
    Hardware::ISerializable::Serialize(out, exponentialCounter);
    Hardware::ISerializable::Serialize(out, exponentialCounterPeriod);
    Hardware::ISerializable::Serialize(out, holdZero);
    Hardware::ISerializable::Serialize(out, gate);
    Hardware::ISerializable::Serialize(out, attackRate);
    Hardware::ISerializable::Serialize(out, decayRate);
    Hardware::ISerializable::Serialize(out, sustainLevel);
    Hardware::ISerializable::Serialize(out, releaseRate);
    Hardware::ISerializable::Serialize(out, targetAttackRate);
    Hardware::ISerializable::Serialize(out, targetDecayRate);
    Hardware::ISerializable::Serialize(out, targetSustainLevel);
    Hardware::ISerializable::Serialize(out, targetReleaseRate);
    return out.good();
}

bool ADSREnvelope::LoadState(std::istream& inStream) {
    Hardware::ISerializable::Deserialize(inStream, state);
    Hardware::ISerializable::Deserialize(inStream, level);
    Hardware::ISerializable::Deserialize(inStream, rateCounter);
    Hardware::ISerializable::Deserialize(inStream, ratePeriod);
    Hardware::ISerializable::Deserialize(inStream, exponentialCounter);
    Hardware::ISerializable::Deserialize(inStream, exponentialCounterPeriod);
    Hardware::ISerializable::Deserialize(inStream, holdZero);
    Hardware::ISerializable::Deserialize(inStream, gate);
    Hardware::ISerializable::Deserialize(inStream, attackRate);
    Hardware::ISerializable::Deserialize(inStream, decayRate);
    Hardware::ISerializable::Deserialize(inStream, sustainLevel);
    Hardware::ISerializable::Deserialize(inStream, releaseRate);
    Hardware::ISerializable::Deserialize(inStream, targetAttackRate);
    Hardware::ISerializable::Deserialize(inStream, targetDecayRate);
    Hardware::ISerializable::Deserialize(inStream, targetSustainLevel);
    Hardware::ISerializable::Deserialize(inStream, targetReleaseRate);
    return inStream.good();
}

}  // namespace Hardware
