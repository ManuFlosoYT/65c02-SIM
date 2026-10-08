#include "SIDResampler.h"

namespace Hardware {

SIDResampler::SIDResampler(double inRate, double outRate) {
    SetRates(inRate, outRate);
}

void SIDResampler::SetRates(double inRate, double outRate) {
    inputRate = inRate;
    outputRate = outRate;
    ratio = inputRate / outputRate;
    
    ringBufferSize = static_cast<std::size_t>(FILTER_TAPS * ratio) + 2;
    numInputTaps = static_cast<std::size_t>(FILTER_TAPS * ratio);
    ringBuffer.assign(ringBufferSize, 0.0);
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
    outputReadIndex = 0;
    outputWriteIndex = 0;
    outputCount = 0;
}

void SIDResampler::PushSample(double sample) {
    ringBuffer[ringIndex] = sample;
    ++ringIndex;
    if (ringIndex == ringBufferSize) ringIndex = 0;
    
    fractionalTime += 1.0;
    
    if (fractionalTime >= ratio) {
        fractionalTime -= ratio;

        double phase = fractionalTime / ratio;
        int phaseIdx = static_cast<int>(phase * PHASES);
        if (phaseIdx >= PHASES) phaseIdx = PHASES - 1;
        if (phaseIdx < 0) phaseIdx = 0;
        
        double outSample = 0.0;
        std::size_t readIdx = ringIndex + ringBufferSize - numInputTaps;
        if (readIdx >= ringBufferSize) readIdx -= ringBufferSize;
        
        const double* tablePtr = &firTable[phaseIdx * numInputTaps];
        
        for (std::size_t i = 0; i < numInputTaps; ++i) {
            outSample += ringBuffer[readIdx] * tablePtr[i];
            readIdx++;
            if (readIdx == ringBufferSize) readIdx = 0;
        }
        
        if (outputCount == outputBuffer.size()) {
            std::vector<double> expandedBuffer(outputBuffer.size() * 2);
            for (std::size_t i = 0; i < outputCount; ++i) {
                expandedBuffer[i] = outputBuffer[(outputReadIndex + i) % outputBuffer.size()];
            }
            outputBuffer = std::move(expandedBuffer);
            outputReadIndex = 0;
            outputWriteIndex = outputCount;
        }
        outputBuffer[outputWriteIndex] = outSample;
        outputWriteIndex = (outputWriteIndex + 1) % outputBuffer.size();
        ++outputCount;
    }
}

bool SIDResampler::HasOutput() const {
    return outputCount != 0;
}

double SIDResampler::GetOutput() {
    if (outputCount == 0) return 0.0;
    double val = outputBuffer[outputReadIndex];
    outputReadIndex = (outputReadIndex + 1) % outputBuffer.size();
    --outputCount;
    return val;
}

} // namespace Hardware
