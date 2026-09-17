#include "render/ExportSettings.hpp"
#include "render/HardwareCapabilities.hpp"

#include <gtest/gtest.h>

#include <limits>

TEST(ExportSettingsTest, DefaultsAreValid) {
    ccos::render::ExportSettings settings;
    QString error;
    EXPECT_TRUE(settings.validate(&error));
    EXPECT_TRUE(error.isEmpty());
}

TEST(ExportSettingsTest, AutomaticVideoCodecIsValid) {
    ccos::render::ExportSettings settings;
    settings.videoCodec = QStringLiteral("auto");
    EXPECT_TRUE(settings.validate());
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

TEST(HardwareCapabilitiesTest, FallsBackToSoftwareWhenHardwareIsDisabledOrUnavailable) {
    ccos::render::HardwareCapabilities capabilities;
    EXPECT_EQ(capabilities.preferredH264Encoder(false), QStringLiteral("libx264"));
    EXPECT_EQ(capabilities.preferredHevcEncoder(false), QStringLiteral("libx265"));
    EXPECT_EQ(capabilities.preferredH264Encoder(true), QStringLiteral("libx264"));
    EXPECT_EQ(capabilities.preferredHevcEncoder(true), QStringLiteral("libx265"));
}

TEST(HardwareCapabilitiesTest, SelectsAvailableLowFrictionHardwareEncoder) {
    ccos::render::HardwareCapabilities capabilities;
    capabilities.encoders = {
        QStringLiteral("h264_videotoolbox"),
        QStringLiteral("hevc_vaapi")
    };

    EXPECT_EQ(capabilities.preferredH264Encoder(), QStringLiteral("h264_videotoolbox"));
    // VAAPI requires a dedicated hardware-device/upload pipeline, so the
    // current generic exporter intentionally falls back to libx265.
    EXPECT_EQ(capabilities.preferredHevcEncoder(), QStringLiteral("libx265"));
}

TEST(HardwareCapabilitiesTest, SupportsAndFlagsRemainConsistent) {
    ccos::render::HardwareCapabilities capabilities;
    capabilities.encoders = {
        QStringLiteral("h264_nvenc"),
        QStringLiteral("hevc_nvenc"),
        QStringLiteral("h264_qsv")
    };
    capabilities.hasNvidia = true;
    capabilities.hasIntel = true;

    EXPECT_TRUE(capabilities.supports(QStringLiteral("h264_nvenc")));
    EXPECT_TRUE(capabilities.supports(QStringLiteral("hevc_nvenc")));
    EXPECT_FALSE(capabilities.supports(QStringLiteral("h264_amf")));
    EXPECT_EQ(capabilities.preferredH264Encoder(), QStringLiteral("h264_nvenc"));
    EXPECT_EQ(capabilities.preferredHevcEncoder(), QStringLiteral("hevc_nvenc"));
}
