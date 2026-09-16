#include "SIDResampler.h"

namespace Hardware {

SIDResampler::SIDResampler(double inRate, double outRate) {
    SetRates(inRate, outRate);
}

void SIDResampler::SetRates(double inRate, double outRate) {
    inputRate = inRate;
    outputRate = outRate;
    ratio = inputRate / outputRate;
    
    int requiredBufferSize = static_cast<int>(FILTER_TAPS * ratio) + 2;
    ringBuffer.assign(requiredBufferSize, 0.0);
    ringIndex = 0;
    fractionalTime = 0.0;
    
    while(!outputQueue.empty()) outputQueue.pop();
    
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
    
    int numInputTaps = static_cast<int>(FILTER_TAPS * ratio);
    int halfTaps = numInputTaps / 2;
    
    firTable.assign(numInputTaps * PHASES, 0.0);
    
    double kaiserBeta = 7.0; // Good stopband attenuation (~70dB)
    double i0Beta = BesselI0(kaiserBeta);
    
    for (int p = 0; p < PHASES; ++p) {
        double phaseOffset = static_cast<double>(p) / PHASES;
        
        double sum = 0.0;
        for (int i = 0; i < numInputTaps; ++i) {
            double x = (i - halfTaps) - phaseOffset;
            
            // Sinc
            double sinc = 1.0;
            if (x != 0.0) {
                double piX = 3.14159265358979323846 * x * 2.0 * cutoff;
                sinc = std::sin(piX) / piX;
            }
            
            // Kaiser Window
            double window = 0.0;
            double arg = (2.0 * x) / numInputTaps;
            if (arg >= -1.0 && arg <= 1.0) {
                window = BesselI0(kaiserBeta * std::sqrt(1.0 - arg * arg)) / i0Beta;
            }
            
            double coeff = sinc * window;
            firTable[p * numInputTaps + i] = coeff;
            sum += coeff;
        }
        
        // Normalize phase
        if (sum > 0.0) {
            for (int i = 0; i < numInputTaps; ++i) {
                firTable[p * numInputTaps + i] /= sum;
            }
        }
    }
}

void SIDResampler::Clear() {
    std::fill(ringBuffer.begin(), ringBuffer.end(), 0.0);
    ringIndex = 0;
    fractionalTime = 0.0;
    while(!outputQueue.empty()) outputQueue.pop();
}

void SIDResampler::PushSample(double sample) {
    ringBuffer[ringIndex] = sample;
    ringIndex = (ringIndex + 1) % ringBuffer.size();
    
    fractionalTime += 1.0;
    
    if (fractionalTime >= ratio) {
        fractionalTime -= ratio;
        
        int numInputTaps = static_cast<int>(FILTER_TAPS * ratio);
        
        double phase = fractionalTime / ratio;
        int phaseIdx = static_cast<int>(phase * PHASES);
        if (phaseIdx >= PHASES) phaseIdx = PHASES - 1;
        if (phaseIdx < 0) phaseIdx = 0;
        
        double outSample = 0.0;
        size_t readIdx = (ringIndex + ringBuffer.size() - numInputTaps) % ringBuffer.size();
        
        const double* tablePtr = &firTable[phaseIdx * numInputTaps];
        
        for (int i = 0; i < numInputTaps; ++i) {
            outSample += ringBuffer[readIdx] * tablePtr[i];
            readIdx++;
            if (readIdx >= ringBuffer.size()) readIdx = 0;
        }
        
        outputQueue.push(outSample);
    }
}

bool SIDResampler::HasOutput() const {
    return !outputQueue.empty();
}

double SIDResampler::GetOutput() {
    if (outputQueue.empty()) return 0.0;
    double val = outputQueue.front();
    outputQueue.pop();
    return val;
}

} // namespace Hardware
