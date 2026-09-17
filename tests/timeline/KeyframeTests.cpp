#include "timeline/Keyframe.hpp"
#include <gtest/gtest.h>

TEST(KeyframeTests, InterpolatesLinearly) {
    ccos::timeline::KeyframeTrack track;
    track.add({ccos::core::Time::fromSeconds(0.0), 0.0, ccos::timeline::Interpolation::Linear});
    track.add({ccos::core::Time::fromSeconds(2.0), 100.0, ccos::timeline::Interpolation::Linear});
    EXPECT_DOUBLE_EQ(track.evaluate(ccos::core::Time::fromSeconds(1.0)), 50.0);
}
