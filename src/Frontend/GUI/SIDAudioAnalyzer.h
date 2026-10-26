#pragma once

#include <array>
#include <vector>
#include <complex>
#include <cstdint>
#include "Hardware/Audio/SID.h"

namespace GUI {

class SIDAudioAnalyzer {
public:
    static constexpr size_t NUM_BANDS = 32;
    static constexpr size_t FILTER_POINTS = 100;

    struct FilterResponse {
        double cutoffHz = 0.0;
        double filterQ = 0.0;
        std::array<float, FILTER_POINTS> yNorms{};
    };

    SIDAudioAnalyzer() = default;

    void ProcessFFT(const Hardware::SID& sid,
                    std::array<float, NUM_BANDS>& spectrumOut,
                    std::array<std::array<float, 3>, NUM_BANDS>& voiceSharesOut,
                    bool should_update);

    static void ComputeFFT(std::vector<std::complex<float>>& a);

    static void ComputeVoiceShares(const Hardware::SID& sid,
                                   std::array<std::array<float, 3>, NUM_BANDS>& voiceShares);

    static void UpdatePeakHold(const std::array<float, NUM_BANDS>& spectrum,
                               std::array<float, NUM_BANDS>& peakHold,
                               std::array<float, NUM_BANDS>& peakTimers,
                               float dt);

    static FilterResponse ComputeFilterResponse(uint16_t fc, uint8_t res, Hardware::SIDModel model,
                                                bool modeLP, bool modeBP, bool modeHP);

private:
    std::array<float, NUM_BANDS> smoothedSpectrum{};
};

} // namespace GUI
