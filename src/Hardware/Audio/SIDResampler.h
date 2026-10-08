#pragma once
#include <cmath>
#include <cstdint>
#include <vector>

namespace Hardware {

class SIDResampler {
public:
    SIDResampler(double inputRate = 1000000.0, double outputRate = 48000.0);
    ~SIDResampler() = default;

    void SetRates(double inputRate, double outputRate);
    
    // Add a sample at the input rate
    void PushSample(double sample);
    
    // Check if we have an output sample ready
    bool HasOutput() const;
    
    // Get the next output sample and consume it
    double GetOutput();

    void Clear();

private:
    void BuildPolyphaseTable();
    double BesselI0(double x);

    double inputRate;
    double outputRate;
    double ratio;

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
