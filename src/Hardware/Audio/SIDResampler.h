#pragma once
#include <cmath>
#include <cstdint>
#include <vector>

namespace Hardware {

class SIDResampler {
public:
    SIDResampler(double inputRate = 1000000.0, double outputRate = 48000.0);
    ~SIDResampler() = default;

    void SetRates(double inRate, double outRate);
    
    // Add a sample at the input rate
    void PushSample(double sample) {
        if (decimFactor > 1) {
            decimAccum += sample;
            if (++decimCount < decimFactor) [[likely]] {
                return;
            }
            sample = decimAccum * decimInv;
            decimAccum = 0.0;
            decimCount = 0;
        }

        double* ringData = ringBuffer.data();
        *(ringData + ringIndex) = sample;
        *(ringData + ringIndex + ringBufferSize) = sample;
        ++ringIndex;
        if (ringIndex == ringBufferSize) [[unlikely]] {
            ringIndex = 0;
        }

        fractionalTime += 1.0;
        if (fractionalTime >= ratio) [[unlikely]] {
            ProduceOutputSample();
        }
    }
    
    // Check if we have an output sample ready
    [[nodiscard]] bool HasOutput() const {
        return outputCount != 0;
    }
    
    // Get the next output sample and consume it
    double GetOutput() {
        if (outputCount == 0) {
            return 0.0;
        }
        double val = *(outputBuffer.data() + outputReadIndex);
        outputReadIndex = (outputReadIndex + 1) & (outputBuffer.size() - 1);
        --outputCount;
        return val;
    }

    void Clear();

private:
    void BuildPolyphaseTable();
    void ProduceOutputSample();
    double BesselI0(double x);

    double inputRate = 1000000.0;
    double outputRate = 48000.0;
    double ratio = 1000000.0 / 48000.0;

    std::size_t decimFactor = 1;
    std::size_t decimCount = 0;
    double decimInv = 1.0;
    double decimAccum = 0.0;

    // FIR Filter state
    std::vector<double> firTable;
    std::vector<double> ringBuffer;
    std::size_t ringBufferSize = 0;
    std::size_t ringIndex = 0;
    std::size_t numInputTaps = 0;
    
    // Timing state
    double fractionalTime = 0.0;
    
    // Output ring buffer
    std::vector<double> outputBuffer;
    std::size_t outputReadIndex = 0;
    std::size_t outputWriteIndex = 0;
    std::size_t outputCount = 0;
    
    // Polyphase configuration
    static constexpr int PHASES = 256;
    static constexpr int FILTER_HALF_TAPS = 16; 
    static constexpr int FILTER_TAPS = FILTER_HALF_TAPS * 2;
};

} // namespace Hardware
