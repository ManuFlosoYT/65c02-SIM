#include "SIDResampler.h"

namespace Hardware {

SIDResampler::SIDResampler(double inRate, double outRate) {
    SetRates(inRate, outRate);
}

void SIDResampler::SetRates(double inRate, double outRate) {
    if (!firTable.empty() && inRate == inputRate * static_cast<double>(decimFactor) && outRate == outputRate) {
        Clear();
        return;
    }

    decimFactor = (inRate >= 2000000.0) ? static_cast<std::size_t>(inRate / 1000000.0) : 1;
    decimCount = 0;
    decimInv = 1.0 / static_cast<double>(decimFactor);
    decimAccum = 0.0;

    inputRate = inRate / static_cast<double>(decimFactor);
    outputRate = outRate;
    ratio = inputRate / outputRate;
    
    ringBufferSize = static_cast<std::size_t>(FILTER_TAPS * ratio) + 2;
    numInputTaps = static_cast<std::size_t>(FILTER_TAPS * ratio);
    ringBuffer.assign(ringBufferSize * 2, 0.0);
    ringIndex = 0;
    fractionalTime = 0.0;

    outputBuffer.assign(1024, 0.0);
    outputReadIndex = 0;
    outputWriteIndex = 0;
    outputCount = 0;
    
    BuildPolyphaseTable();
}

double SIDResampler::BesselI0(double x) {
    double sum = 1.0;
    double term = 1.0;
    double x2_4 = (x * x) / 4.0;
    for (int i = 1; i < 20; ++i) {
        term *= x2_4 / (i * i);
        sum += term;
        if (term < 1e-12 * sum) break;
    }
    return sum;
}

void SIDResampler::BuildPolyphaseTable() {
    double cutoff = (outputRate * 0.45) / inputRate; // 90% of Nyquist to give room for transition
    
    const int tapCount = static_cast<int>(numInputTaps);
    int halfTaps = tapCount / 2;
    
    firTable.assign(numInputTaps * PHASES, 0.0);
    
    double kaiserBeta = 7.0; // Good stopband attenuation (~70dB)
    double i0Beta = BesselI0(kaiserBeta);
    
    for (int p = 0; p < PHASES; ++p) {
        double phaseOffset = static_cast<double>(p) / PHASES;
        
        double sum = 0.0;
        for (int i = 0; i < tapCount; ++i) {
            double x = (i - halfTaps) - phaseOffset;
            
            // Sinc
            double sinc = 1.0;
            if (x != 0.0) {
                double piX = 3.14159265358979323846 * x * 2.0 * cutoff;
                sinc = std::sin(piX) / piX;
            }
            
            // Kaiser Window
            double window = 0.0;
            double arg = (2.0 * x) / tapCount;
            if (arg >= -1.0 && arg <= 1.0) {
                window = BesselI0(kaiserBeta * std::sqrt(1.0 - arg * arg)) / i0Beta;
            }
            
            double coeff = sinc * window;
            firTable[p * numInputTaps + i] = coeff;
            sum += coeff;
        }
        
        // Normalize phase
        if (sum > 0.0) {
            for (int i = 0; i < tapCount; ++i) {
                firTable[p * numInputTaps + i] /= sum;
            }
        }
    }
}

void SIDResampler::Clear() {
    std::fill(ringBuffer.begin(), ringBuffer.end(), 0.0);
    ringIndex = 0;
    fractionalTime = 0.0;
    decimCount = 0;
    decimAccum = 0.0;
    outputReadIndex = 0;
    outputWriteIndex = 0;
    outputCount = 0;
}

void SIDResampler::ProduceOutputSample() {
    fractionalTime -= ratio;

    double phase = fractionalTime / ratio;
    int phaseIdx = std::clamp(static_cast<int>(phase * PHASES), 0, PHASES - 1);

    double outSample = 0.0;
    std::size_t readIdx = ringIndex + ringBufferSize - numInputTaps;
    if (readIdx >= ringBufferSize) {
        readIdx -= ringBufferSize;
    }

    const double* tablePtr = firTable.data() + (static_cast<std::size_t>(phaseIdx) * numInputTaps);
    const double* samplePtr = ringBuffer.data() + readIdx;
    const double* const tableEnd = tablePtr + numInputTaps;

    while (tablePtr != tableEnd) {
        outSample += *samplePtr++ * *tablePtr++;
    }

    if (outputCount == outputBuffer.size()) [[unlikely]] {
        const std::size_t outputBufferCapacity = outputBuffer.size();
        const std::size_t mask = outputBufferCapacity - 1;
        std::vector<double> expandedBuffer(outputBufferCapacity * 2);
        double* const expData = expandedBuffer.data();
        const double* const outData = outputBuffer.data();
        for (std::size_t i = 0; i < outputCount; ++i) {
            *(expData + i) = *(outData + ((outputReadIndex + i) & mask));
        }
        outputBuffer = std::move(expandedBuffer);
        outputReadIndex = 0;
        outputWriteIndex = outputCount;
    }
    const std::size_t mask = outputBuffer.size() - 1;
    *(outputBuffer.data() + outputWriteIndex) = outSample;
    outputWriteIndex = (outputWriteIndex + 1) & mask;
    ++outputCount;
}

} // namespace Hardware
