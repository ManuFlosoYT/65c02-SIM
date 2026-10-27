#include "Hardware/Audio/Oscillator.h"
#include "Hardware/Audio/SIDWaveTables.h"
#include "Hardware/Core/ISerializable.h"

#include <iostream>

namespace Hardware {

void Oscillator::Next(SIDModel model) {
    if ((control & 0x08) != 0) [[unlikely]] {
        accumulator = 0;
        noiseShift = 0x7FFFF8;
        oscOutput = 0;
        return;
    }

    uint32_t prevAcc = accumulator;
    accumulator = (accumulator + frequency) & 0xFFFFFF;

    if ((control & 0x02) != 0 && prevOsc != nullptr) [[unlikely]] {
        if (prevOsc->accumulator < prevOsc->frequency) {
            accumulator = 0;
        }
    }

    uint8_t waveControl = (control >> 4) & 0x0F;
    if (waveControl == 0) {
        oscOutput = 0;
        return;
    }

    uint16_t out = 0;
    bool noiseOn = (waveControl & 0x08) != 0;
    bool pulseOn = (waveControl & 0x04) != 0;
    bool sawOn = (waveControl & 0x02) != 0;
    bool triOn = (waveControl & 0x01) != 0;

    if (noiseOn) {
        if ((accumulator & 0x80000) != (prevAcc & 0x80000)) {
            uint32_t bit = ((noiseShift >> 22) ^ (noiseShift >> 17)) & 1;
            noiseShift = ((noiseShift << 1) & 0x7FFFFF) | bit;
        }
    }

    if (noiseOn) {
        uint16_t noise = (
            ((noiseShift & 0x400000) >> 11) |
            ((noiseShift & 0x100000) >> 10) |
            ((noiseShift & 0x010000) >> 7)  |
            ((noiseShift & 0x002000) >> 5)  |
            ((noiseShift & 0x000800) >> 4)  |
            ((noiseShift & 0x000080) >> 1)  |
            ((noiseShift & 0x000010) << 1)  |
            ((noiseShift & 0x000004) << 2)
        ) & 0xFFF;

        if (model == SIDModel::MOS6581 && waveControl != 0x08) {
            out = 0;
        } else {
            out = noise;
        }
    } else {
        uint32_t ringMask = ((control & 0x04) != 0 && prevOsc) ? 0x800000 : 0;
        uint32_t syncAcc = prevOsc ? prevOsc->accumulator : 0;
        uint16_t ix = (accumulator ^ (~syncAcc & ringMask)) >> 12;

        uint16_t pulseOutput = ((accumulator >> 12) >= pulseWidth) ? 0xFFF : 0;

        if (model == SIDModel::MOS6581) {
            if (waveControl == 0x01) {
                uint32_t msb = accumulator & 0x800000;
                if (ringMask) msb ^= (syncAcc & 0x800000);
                out = ((accumulator ^ (msb ? 0xFFFFFF : 0)) >> 11) & 0xFFF;
            } else if (waveControl == 0x02) {
                out = ix;
            } else if (waveControl == 0x04) {
                out = pulseOutput;
            } else if (waveControl == 0x03) {
                out = wave6581__ST[ix];
            } else if (waveControl == 0x05) {
                out = wave6581_P_T[ix] & (pulseOutput | ~0xFFF);
            } else if (waveControl == 0x06) {
                out = wave6581_PS_[ix] & (pulseOutput | ~0xFFF);
            } else if (waveControl == 0x07) {
                out = wave6581_PST[ix] & (pulseOutput | ~0xFFF);
            }
        } else {
            out = 0xFFF;
            if (triOn) {
                uint32_t msb = accumulator & 0x800000;
                if (ringMask) msb ^= (syncAcc & 0x800000);
                out &= ((accumulator ^ (msb ? 0xFFFFFF : 0)) >> 11) & 0xFFF;
            }
            if (sawOn) out &= ix;
            if (pulseOn) out &= pulseOutput;
        }
    }

    oscOutput = out;
}

bool Oscillator::SaveState(std::ostream& out) const {
    Hardware::ISerializable::Serialize(out, accumulator);
    Hardware::ISerializable::Serialize(out, frequency);
    Hardware::ISerializable::Serialize(out, pulseWidth);
    Hardware::ISerializable::Serialize(out, control);
    Hardware::ISerializable::Serialize(out, noiseShift);
    Hardware::ISerializable::Serialize(out, oscOutput);
    env.SaveState(out);
    return out.good();
}

bool Oscillator::LoadState(std::istream& inStream) {
    Hardware::ISerializable::Deserialize(inStream, accumulator);
    Hardware::ISerializable::Deserialize(inStream, frequency);
    Hardware::ISerializable::Deserialize(inStream, pulseWidth);
    Hardware::ISerializable::Deserialize(inStream, control);
    Hardware::ISerializable::Deserialize(inStream, noiseShift);
    Hardware::ISerializable::Deserialize(inStream, oscOutput);
    env.LoadState(inStream);
    return inStream.good();
}

}  // namespace Hardware
