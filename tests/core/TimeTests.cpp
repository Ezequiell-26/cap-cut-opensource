#include "core/Time.hpp"
#include <gtest/gtest.h>

TEST(TimeTests, NormalizesFractions) {
    const ccos::core::Time time(120, 60);
    EXPECT_EQ(time.numerator(), 2);
    EXPECT_EQ(time.denominator(), 1);
}

TEST(TimeTests, FrameConversionIsExact) {
    const auto time = ccos::core::Time::fromFrames(30, 30);
    EXPECT_EQ(time.numerator(), 1);
    EXPECT_EQ(time.denominator(), 1);
}

TEST(TimeTests, AdditionWorksAcrossDenominators) {
    const auto result = ccos::core::Time(1, 2) + ccos::core::Time(1, 3);
    EXPECT_EQ(result.numerator(), 5);
    EXPECT_EQ(result.denominator(), 6);
}


TEST(TimeTests, RejectsOutOfRangeSecondConversion) {
    const auto result = ccos::core::Time::fromSeconds(1.0e20);
    EXPECT_EQ(result, ccos::core::Time{});
}
