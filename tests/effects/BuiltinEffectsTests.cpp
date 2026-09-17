#include "effects/BuiltinEffects.hpp"
#include <gtest/gtest.h>

TEST(BuiltinEffectsTests, ListsCoreEffects) {
    const auto ids = ccos::effects::BuiltinEffects::ids();
    EXPECT_TRUE(ids.contains(QStringLiteral("brightness")));
    EXPECT_TRUE(ids.contains(QStringLiteral("contrast")));
    EXPECT_TRUE(ids.contains(QStringLiteral("blur")));
}

TEST(BuiltinEffectsTests, ProducesFilters) {
    EXPECT_EQ(ccos::effects::BuiltinEffects::ffmpegFilter(QStringLiteral("grayscale")), QStringLiteral("hue=s=0"));
    EXPECT_FALSE(ccos::effects::BuiltinEffects::ffmpegFilter(QStringLiteral("brightness")).isEmpty());
}
