#include "render/ExportPresets.hpp"

#include <gtest/gtest.h>

TEST(ExportPresetsTest, CatalogContainsStablePresetIds) {
    const auto presets = ccos::render::ExportPresetCatalog::all();
    ASSERT_EQ(presets.size(), 5);
    for (const auto& preset : presets) {
        EXPECT_FALSE(preset.name.isEmpty());
        EXPECT_FALSE(preset.description.isEmpty());
        EXPECT_TRUE(preset.settings.validate());
        EXPECT_FALSE(ccos::render::ExportPresetCatalog::idString(preset.id).isEmpty());
    }
}

TEST(ExportPresetsTest, PresetFactoriesReturnExpectedGeometry) {
    const auto vertical = ccos::render::ExportPresetCatalog::settingsFor(
        ccos::render::ExportPresetId::H264_Vertical1080p30);
    EXPECT_EQ(vertical.width, 1080);
    EXPECT_EQ(vertical.height, 1920);
    EXPECT_EQ(vertical.container, QStringLiteral("mp4"));

    const auto fourK = ccos::render::ExportPresetCatalog::settingsFor(
        ccos::render::ExportPresetId::H264_4K30);
    EXPECT_EQ(fourK.width, 3840);
    EXPECT_EQ(fourK.height, 2160);
}

TEST(ExportPresetsTest, WebMPresetUsesWebCodecs) {
    const auto settings = ccos::render::ExportPresetCatalog::settingsFor(
        ccos::render::ExportPresetId::WebM_1080p30);
    EXPECT_EQ(settings.container, QStringLiteral("webm"));
    EXPECT_EQ(settings.videoCodec, QStringLiteral("libvpx-vp9"));
    EXPECT_EQ(settings.audioCodec, QStringLiteral("libopus"));
    EXPECT_TRUE(settings.validate());
}

TEST(ExportPresetsTest, PresetIdsRoundTripCaseInsensitively) {
    const auto expected = ccos::render::ExportPresetId::H264_1080p60;
    const QString id = ccos::render::ExportPresetCatalog::idString(expected);

    ccos::render::ExportPresetId actual = ccos::render::ExportPresetId::H264_1080p30;
    ASSERT_TRUE(ccos::render::ExportPresetCatalog::fromIdString(id.toUpper(), &actual));
    EXPECT_EQ(actual, expected);
    EXPECT_FALSE(ccos::render::ExportPresetCatalog::fromIdString(QStringLiteral("unknown"), &actual));
}
