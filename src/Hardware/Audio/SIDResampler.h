#pragma once
#include <vector>
#include <cmath>
#include <cstdint>
#include <queue>

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
    size_t ringIndex;
    
    // Timing state
    double fractionalTime;
    
    // Output queue
    std::queue<double> outputQueue;
    
    // Polyphase configuration
    static constexpr int PHASES = 256;
    static constexpr int FILTER_HALF_TAPS = 16; 
    static constexpr int FILTER_TAPS = FILTER_HALF_TAPS * 2;
};

} // namespace Hardware
