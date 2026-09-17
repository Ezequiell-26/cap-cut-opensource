#include "render/ExportSettings.hpp"

#include <gtest/gtest.h>

#include <limits>

TEST(ExportSettingsTest, DefaultsAreValid) {
    ccos::render::ExportSettings settings;
    QString error;
    EXPECT_TRUE(settings.validate(&error));
    EXPECT_TRUE(error.isEmpty());
}

TEST(ExportSettingsTest, RejectsUnsafeNumericValues) {
    ccos::render::ExportSettings settings;
    settings.width = 1;
    EXPECT_FALSE(settings.validate());

    settings.width = 1920;
    settings.fps = std::numeric_limits<double>::infinity();
    EXPECT_FALSE(settings.validate());

    settings.fps = 30.0;
    settings.videoBitrateKbps = 2'000'000;
    EXPECT_FALSE(settings.validate());
}

TEST(ExportSettingsTest, RejectsArgumentLikeTokens) {
    ccos::render::ExportSettings settings;

    settings.videoCodec = QStringLiteral("-filter_complex");
    EXPECT_FALSE(settings.validate());

    settings.videoCodec = QStringLiteral("libx264");
    settings.audioCodec = QStringLiteral("aac codec");
    EXPECT_FALSE(settings.validate());

    settings.audioCodec = QStringLiteral("aac");
    settings.container = QStringLiteral("mp4/../mkv");
    EXPECT_FALSE(settings.validate());
}
