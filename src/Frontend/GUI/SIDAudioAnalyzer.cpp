#include "Frontend/GUI/SIDAudioAnalyzer.h"
#include <cmath>
#include <numbers>
#include <algorithm>

namespace GUI {

void SIDAudioAnalyzer::ComputeFFT(std::vector<std::complex<float>>& a) {
    const size_t n = a.size();
    for (size_t i = 1, j = 0; i < n; ++i) {
        size_t bit = n >> 1U;
        for (; (j & bit) != 0; bit >>= 1U) {
            j ^= bit;
        }
        j ^= bit;
        if (i < j) {
            std::swap(a[i], a[j]);
        }
    }
    for (size_t len = 2; len <= n; len <<= 1U) {
        float ang = -2.0F * std::numbers::pi_v<float> / static_cast<float>(len);
        std::complex<float> wlen(std::cos(ang), std::sin(ang));
        for (size_t i = 0; i < n; i += len) {
            std::complex<float> w(1.0F, 0.0F);
            for (size_t j = 0; j < (len / 2U); ++j) {
                std::complex<float> u = a[i + j];
                std::complex<float> v = a[i + j + (len / 2U)] * w;
                a[i + j] = u + v;
                a[i + j + (len / 2U)] = u - v;
                w *= wlen;
            }
        }
    }
}

void SIDAudioAnalyzer::ComputeVoiceShares(const Hardware::SID& sid, std::array<std::array<float, 3>, NUM_BANDS>& voiceShares) {
    std::array<std::array<float, 3>, NUM_BANDS> energy{};
    for (int v = 0; v < 3; ++v) {
        if (sid.IsVoiceMuted(v)) {
            continue;
        }
        if ((sid.IsVoiceSolo(0) || sid.IsVoiceSolo(1) || sid.IsVoiceSolo(2)) && !sid.IsVoiceSolo(v)) {
            continue;
        }
        const auto& voice = sid.GetVoice(v);
        if (voice.frequency == 0 || voice.env.level == 0 || (voice.control & 0x08) != 0) {
            continue;
        }
        const uint8_t waveControl = (voice.control >> 4) & 0x0F;
        if (waveControl == 0) {
            continue;
        }

        const float envLevel = static_cast<float>(voice.env.level) / 255.0F;
        if ((waveControl & 0x08) != 0) {
            for (size_t i = 0; i < NUM_BANDS; ++i) {
                energy.at(i).at(static_cast<size_t>(v)) += envLevel * 0.35F;
            }
            continue;
        }

        const float freqHz = static_cast<float>(voice.frequency) * (1000000.0F / 16777216.0F);
        if (freqHz < 16.0F) {
            continue;
        }

        constexpr float binHz = 48000.0F / 128.0F;
        const float pulseDuty = std::clamp(static_cast<float>(voice.pulseWidth & 0x0FFF) / 4095.0F, 0.05F, 0.95F);
        const int maxHarmonic = std::min(64, static_cast<int>(16000.0F / freqHz));

        for (int k = 1; k <= maxHarmonic; ++k) {
            const float kF = static_cast<float>(k);
            const float hFreq = freqHz * kF;
            float hAmp = 0.0F;
            if ((waveControl & 0x01) != 0 && (k % 2 != 0)) {
                hAmp = std::max(hAmp, 1.0F / (kF * kF));
            }
            if ((waveControl & 0x02) != 0) {
                hAmp = std::max(hAmp, 1.0F / kF);
            }
            if ((waveControl & 0x04) != 0) {
                hAmp = std::max(hAmp, std::abs(std::sin(kF * std::numbers::pi_v<float> * pulseDuty)) / kF);
            }
            if (hAmp <= 0.0F) {
                continue;
            }

            const float binIdx = (hFreq / binHz) - 1.0F;
            if (binIdx >= -0.5F && binIdx < 32.5F) {
                const int b0 = static_cast<int>(std::floor(binIdx));
                const int b1 = b0 + 1;
                const float frac = binIdx - static_cast<float>(b0);
                const float totalEnergy = envLevel * hAmp;
                if (b0 >= 0 && b0 < static_cast<int>(NUM_BANDS)) {
                    energy.at(static_cast<size_t>(b0)).at(static_cast<size_t>(v)) += totalEnergy * (1.0F - frac);
                }
                if (b1 >= 0 && b1 < static_cast<int>(NUM_BANDS)) {
                    energy.at(static_cast<size_t>(b1)).at(static_cast<size_t>(v)) += totalEnergy * frac;
                }
            }
        }
    }

    for (size_t i = 0; i < NUM_BANDS; ++i) {
        const float sum = energy.at(i).at(0) + energy.at(i).at(1) + energy.at(i).at(2);
        if (sum > 1e-4F) {
            voiceShares.at(i).at(0) = energy.at(i).at(0) / sum;
            voiceShares.at(i).at(1) = energy.at(i).at(1) / sum;
            voiceShares.at(i).at(2) = energy.at(i).at(2) / sum;
        } else {
            voiceShares.at(i) = {1.0F / 3.0F, 1.0F / 3.0F, 1.0F / 3.0F};
        }
    }
}

void SIDAudioAnalyzer::ProcessFFT(const Hardware::SID& sid, std::array<float, NUM_BANDS>& spectrumOut,
                                  std::array<std::array<float, 3>, NUM_BANDS>& voiceSharesOut, bool should_update) {
    if (!should_update) {
        for (size_t i = 0; i < NUM_BANDS; ++i) {
            smoothedSpectrum.at(i) *= 0.88F;
            if (smoothedSpectrum.at(i) < 0.005F) {
                smoothedSpectrum.at(i) = 0.0F;
            }
            spectrumOut.at(i) = smoothedSpectrum.at(i);
        }
        return;
    }

    constexpr size_t FFT_SIZE = 128;
    std::array<float, FFT_SIZE> samples{};
    sid.GetAudioSnapshot(samples);

    std::vector<std::complex<float>> fftData(FFT_SIZE);
    for (size_t i = 0; i < FFT_SIZE; ++i) {
        float window = 0.5F * (1.0F - std::cos(2.0F * std::numbers::pi_v<float> * static_cast<float>(i) / (FFT_SIZE - 1.0F)));
        fftData[i] = std::complex<float>(samples.at(i) * window, 0.0F);
    }
    ComputeFFT(fftData);

    for (size_t i = 0; i < NUM_BANDS; ++i) {
        float rawMag = (std::abs(fftData.at(i + 1)) / (static_cast<float>(FFT_SIZE) * 0.25F));
        float db = (rawMag > 1e-4F) ? (20.0F * std::log10(rawMag)) : -60.0F;
        float norm = std::clamp((db + 60.0F) / 60.0F, 0.0F, 1.0F);
        smoothedSpectrum.at(i) = std::clamp(std::max(norm, smoothedSpectrum.at(i) * 0.82F), 0.0F, 1.0F);
        spectrumOut.at(i) = smoothedSpectrum.at(i);
    }
    ComputeVoiceShares(sid, voiceSharesOut);
}

void SIDAudioAnalyzer::UpdatePeakHold(const std::array<float, NUM_BANDS>& spectrum,
                                      std::array<float, NUM_BANDS>& peakHold,
                                      std::array<float, NUM_BANDS>& peakTimers,
                                      float dt) {
    for (size_t i = 0; i < NUM_BANDS; ++i) {
        float val = spectrum.at(i);
        if (val >= peakHold.at(i)) {
            peakHold.at(i) = val;
            peakTimers.at(i) = 0.35F;
        } else if (peakTimers.at(i) > 0.0F) {
            peakTimers.at(i) -= dt;
        } else {
            peakHold.at(i) = std::max(val, peakHold.at(i) - (0.65F * dt));
        }
    }
}

SIDAudioAnalyzer::FilterResponse SIDAudioAnalyzer::ComputeFilterResponse(uint16_t fc, uint8_t res, Hardware::SIDModel model,
                                                                        bool modeLP, bool modeBP, bool modeHP) {
    FilterResponse resp;
    const double fcScaled = static_cast<double>(fc) / 2047.0;
    double cutoffHz = (model == Hardware::SIDModel::MOS6581) ? (30.0 + (12000.0 * fcScaled * fcScaled * fcScaled))
                                                            : (30.0 + (fcScaled * 12000.0));
    cutoffHz = std::clamp(cutoffHz, 1.0, 24000.0);

    double filterQ = (model == Hardware::SIDModel::MOS6581)
                         ? (1.5 - ((static_cast<double>(res) / 15.0) * (1.0 - (fcScaled * fcScaled * 0.85))))
                         : (2.0 - (1.8 * (static_cast<double>(res) / 15.0)));
    filterQ = std::max(0.1, filterQ);

    resp.cutoffHz = cutoffHz;
    resp.filterQ = filterQ;

    const double omega0 = 2.0 * std::numbers::pi * cutoffHz;
    const double omega0_sq = omega0 * omega0;

    for (size_t i = 0; i < FILTER_POINTS; ++i) {
        const double t = static_cast<double>(i) / 99.0;
        const double f = 20.0 * std::pow(1000.0, t);
        const double omega = 2.0 * std::numbers::pi * f;
        const double omega_sq = omega * omega;
        const double termD = (omega0 * omega) / filterQ;
        const double denomDiff = omega0_sq - omega_sq;
        const double denom = std::max(1e-9, std::sqrt((denomDiff * denomDiff) + (termD * termD)));

        const double nReal = (modeLP ? omega0_sq : 0.0) - (modeHP ? omega_sq : 0.0);
        const double nImag = modeBP ? termD : 0.0;
        const double num = std::sqrt((nReal * nReal) + (nImag * nImag));
        const double h = (modeLP || modeBP || modeHP) ? (num / denom) : 0.0;
        const double dB = std::clamp(20.0 * std::log10(h + 1e-4), -24.0, 18.0);
        resp.yNorms.at(i) = static_cast<float>((18.0 - dB) / 42.0);
    }

    return resp;
}

} // namespace GUI
