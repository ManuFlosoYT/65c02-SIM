#include "Hardware/Audio/SIDResampler.h"

#include <gtest/gtest.h>

#include <cmath>

using namespace Hardware;

TEST(SIDResamplerTest, ProducesExpectedOutputRate) {
    SIDResampler resampler(1'000'000.0, 48'000.0);
    constexpr int inputSamples = 1'000'000;
    int outputSamples = 0;

    for (int i = 0; i < inputSamples; ++i) {
        resampler.PushSample(0.25);
        while (resampler.HasOutput()) {
            EXPECT_TRUE(std::isfinite(resampler.GetOutput()));
            ++outputSamples;
        }
    }

    EXPECT_NEAR(outputSamples, 48'000, 1);
}

TEST(SIDResamplerTest, ClearDropsPendingOutput) {
    SIDResampler resampler(1'000'000.0, 48'000.0);

    for (int i = 0; i < 1'000'000 / 48; ++i) {
        resampler.PushSample(1.0);
    }

    resampler.Clear();

    EXPECT_FALSE(resampler.HasOutput());
}
