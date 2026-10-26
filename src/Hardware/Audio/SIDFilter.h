#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <numbers>

namespace Hardware {

enum class SIDModel { MOS6581, MOS8580 };

inline double FastTanh(double x) {
    if (x == 0.0) {
        return 0.0;
    }
    if (x > 4.5) {
        return 1.0;
    }
    if (x < -4.5) {
        return -1.0;
    }
    const double x2 = x * x;
    return x * (135135.0 + (x2 * (17325.0 + (x2 * (378.0 + x2))))) /
           (135135.0 + (x2 * (62370.0 + (x2 * (3150.0 + (x2 * 28.0))))));
}

struct SIDFilter {
    double filterLow = 0.0;
    double filterBand = 0.0;
    double filterQ = 1.0;
    double filterG = 0.0;
    double filterR = 1.0;
    double filterRPlusG = 1.0;
    double filterDenomInv = 1.0;
    double dcBlockerState = 0.0;
    double dcBlockerPrevIn = 0.0;
    bool filterCoefficientsDirty = true;

    inline void Reset() {
        filterLow = 0.0;
        filterBand = 0.0;
        filterQ = 1.0;
        filterG = 0.0;
        filterR = 1.0;
        filterRPlusG = 1.0;
        filterDenomInv = 1.0;
        dcBlockerState = 0.0;
        dcBlockerPrevIn = 0.0;
        filterCoefficientsDirty = true;
    }

    [[nodiscard]] inline bool IsSilent() const {
        return filterLow == 0.0 && filterBand == 0.0 && dcBlockerState == 0.0 && dcBlockerPrevIn == 0.0;
    }

    inline void UpdateFilterCoefficients(uint16_t fc, uint8_t res, SIDModel model, double clockRate = 1000000.0) {
        double fcScaled = static_cast<double>(fc) * (1.0 / 2047.0);
        double cutoffHz = 0.0;
        if (model == SIDModel::MOS6581) {
            cutoffHz = 30.0 + (12000.0 * (fcScaled * fcScaled * fcScaled));
            double effectiveRes = (static_cast<double>(res) * (1.0 / 15.0)) * (1.0 - (fcScaled * fcScaled * 0.85));
            effectiveRes = std::max(0.0, effectiveRes);
            filterQ = 1.5 - effectiveRes;
        } else {
            cutoffHz = 30.0 + (fcScaled * 12000.0);
            filterQ = 2.0 - (1.8 * (static_cast<double>(res) * (1.0 / 15.0)));
        }
        cutoffHz = std::clamp(cutoffHz, 1.0, 24000.0);
        double w = 2.0 * std::numbers::pi * cutoffHz / clockRate;
        filterG = std::tan(w * 0.5);
        filterR = 1.0 / filterQ;
        filterRPlusG = filterR + filterG;
        filterDenomInv = 1.0 / (1.0 + (filterG * filterRPlusG));
        filterCoefficientsDirty = false;
    }

    inline double Process(double filteredInput, double directOutput, uint8_t filterMode, uint8_t currentVolume,
                          SIDModel model, bool voicesSilent) {
        double hp = (filteredInput - filterLow - (filterBand * filterRPlusG)) * filterDenomInv;
        double bp = filterBand + (filterG * hp);
        double lp = filterLow + (filterG * bp);

        filterBand = bp + (filterG * hp);
        filterLow = lp + (filterG * bp);

        if (model == SIDModel::MOS6581) {
            filterBand = FastTanh(filterBand);
            filterLow = FastTanh(filterLow);
        }

        double filterOutput = 0.0;
        if ((filterMode & 0x10) != 0) {
            filterOutput += filterLow;
        }
        if ((filterMode & 0x20) != 0) {
            filterOutput += filterBand;
        }
        if ((filterMode & 0x40) != 0) {
            filterOutput += hp;
        }

        if (model == SIDModel::MOS6581) {
            filterOutput += filteredInput * 0.05;
        }

        static constexpr double INV_15 = 1.0 / 15.0;
        double mix = FastTanh((directOutput + filterOutput) * 0.25);
        const double volScaled = static_cast<double>(currentVolume) * INV_15;
        mix *= volScaled;

        if (model == SIDModel::MOS6581) {
            mix += (volScaled - 0.5) * 0.45;
        }

        double pole = (model == SIDModel::MOS6581) ? 0.99995 : 0.995;
        double filteredMix = mix - dcBlockerPrevIn + (pole * dcBlockerState);
        if (std::abs(filteredMix) < 1e-15 && voicesSilent) {
            filteredMix = 0.0;
            filterLow = 0.0;
            filterBand = 0.0;
            dcBlockerState = 0.0;
            dcBlockerPrevIn = 0.0;
        } else {
            dcBlockerState = filteredMix;
            dcBlockerPrevIn = mix;
        }
        return filteredMix;
    }
};

}  // namespace Hardware
