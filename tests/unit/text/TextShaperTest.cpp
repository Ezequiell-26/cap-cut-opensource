#include "text/TextShaper.hpp"

#include <gtest/gtest.h>

TEST(TextShaperTest, EmptyInputReturnsEmptyRun) {
    const auto run = ccos::text::TextShaper::shape(QString());
    EXPECT_TRUE(run.isEmpty());
}

TEST(TextShaperTest, ShapesUnicodeTextWithoutLosingInputOrder) {
    const auto run = ccos::text::TextShaper::shape(QString::fromUtf8("Café 世界"));
    ASSERT_FALSE(run.isEmpty());
    EXPECT_EQ(run.size(), 8);
}

TEST(TextShaperTest, RTLRequestReturnsValidRun) {
    const auto run = ccos::text::TextShaper::shape(QString::fromUtf8("مرحبا"), {}, {}, true);
    EXPECT_FALSE(run.isEmpty());
}
