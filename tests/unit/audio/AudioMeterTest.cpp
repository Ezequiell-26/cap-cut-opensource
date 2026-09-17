#include "audio/AudioMeter.hpp"

#include <gtest/gtest.h>

TEST(AudioMeterTest, ComputesPeakAndRms) {
    const float samples[] = {1.0F, -1.0F, 0.0F, 0.0F};
    const auto reading = ccos::audio::AudioMeter::analyze(samples, 2);
    EXPECT_FLOAT_EQ(reading.peak, 1.0F);
    EXPECT_NEAR(reading.rms, 0.7071067F, 1e-5F);
    EXPECT_NEAR(reading.peakDbfs, 0.0F, 1e-5F);
}
