#include "api/EditorApi.hpp"

#include <gtest/gtest.h>

#include <QJsonArray>
#include <QJsonObject>
#include <limits>

namespace {
ccos::project::Project makeProject() {
    return ccos::project::Project(QStringLiteral("API Test Project"));
}

ccos::timeline::Clip makeClip(double startSeconds, double durationSeconds, double sourceInSeconds = 0.0) {
    ccos::media::MediaAsset asset(QStringLiteral("demo.mp4"));
    asset.metadata().durationMs = 60000;
    ccos::timeline::Clip clip(asset);
    clip.setStart(ccos::core::Time::fromSeconds(startSeconds));
    clip.setSourceRange(ccos::core::Time::fromSeconds(sourceInSeconds),
                        ccos::core::Time::fromSeconds(sourceInSeconds + durationSeconds));
    return clip;
}
}

TEST(EditorApiTest, ExportPresetsAreMachineReadable) {
    const QJsonObject result = ccos::api::EditorApi::exportPresets();

    ASSERT_TRUE(result.value(QStringLiteral("ok")).toBool());
    const QJsonArray presets = result.value(QStringLiteral("presets")).toArray();
    ASSERT_EQ(presets.size(), 5);
    for (const auto& value : presets) {
        const QJsonObject preset = value.toObject();
        EXPECT_FALSE(preset.value(QStringLiteral("id")).toString().isEmpty());
        EXPECT_GT(preset.value(QStringLiteral("width")).toInt(), 0);
        EXPECT_GT(preset.value(QStringLiteral("height")).toInt(), 0);
        EXPECT_GT(preset.value(QStringLiteral("fps")).toDouble(), 0.0);
    }
}

TEST(EditorApiTest, CulturalMediaProvidersAreMachineReadable) {
    const QJsonObject result = ccos::api::EditorApi::culturalMediaProviders();

    ASSERT_TRUE(result.value(QStringLiteral("ok")).toBool());
    const QJsonArray providers = result.value(QStringLiteral("providers")).toArray();
    ASSERT_EQ(providers.size(), 5);
    for (const auto& value : providers) {
        const QJsonObject provider = value.toObject();
        EXPECT_FALSE(provider.value(QStringLiteral("id")).toString().isEmpty());
        EXPECT_FALSE(provider.value(QStringLiteral("auth")).toString().isEmpty());
        EXPECT_FALSE(provider.value(QStringLiteral("rights")).toString().isEmpty());
    }
}

TEST(EditorApiTest, CulturalMediaProviderCatalogIsExposedAsCommand) {
    auto project = makeProject();
    const QJsonObject request{{QStringLiteral("op"), QStringLiteral("cultural_media_providers")}};
    const QJsonObject result = ccos::api::EditorApi::command(project, request);
    EXPECT_TRUE(result.value(QStringLiteral("ok")).toBool());
    EXPECT_EQ(result.value(QStringLiteral("providers")).toArray().size(), 5);
}

TEST(EditorApiTest, UnknownExportPresetFailsBeforeStartingProcess) {
    auto project = makeProject();
    const QJsonObject request{
        {QStringLiteral("op"), QStringLiteral("export")},
        {QStringLiteral("output"), QStringLiteral("/tmp/output.mp4")},
        {QStringLiteral("preset"), QStringLiteral("not-a-real-preset")}
    };

    const QJsonObject result = ccos::api::EditorApi::command(project, request);
    EXPECT_FALSE(result.value(QStringLiteral("ok")).toBool());
    EXPECT_EQ(result.value(QStringLiteral("error")).toString(), QStringLiteral("unknown export preset"));
}

TEST(EditorApiTest, ValidationReportsEmptyProjectWarningsWithoutErrors) {
    const auto project = makeProject();
    const QJsonObject result = ccos::api::EditorApi::validate(project);

    EXPECT_TRUE(result.value(QStringLiteral("ok")).toBool());
    EXPECT_TRUE(result.value(QStringLiteral("errors")).toArray().isEmpty());
    EXPECT_FALSE(result.value(QStringLiteral("warnings")).toArray().isEmpty());
}

TEST(EditorApiTest, AutomatedSlipMutatesOnlyRequestedClip) {
    auto project = makeProject();
    auto& track = project.timeline().ensureVideoTrack();
    track.addClip(makeClip(0.0, 5.0, 10.0));
    track.addClip(makeClip(5.0, 4.0, 30.0));

    const QJsonObject request{
        {QStringLiteral("op"), QStringLiteral("slip")},
        {QStringLiteral("trackIndex"), 0},
        {QStringLiteral("clipIndex"), 1},
        {QStringLiteral("sourceDeltaMs"), 1500}
    };
    const QJsonObject result = ccos::api::EditorApi::command(project, request);

    ASSERT_TRUE(result.value(QStringLiteral("ok")).toBool());
    EXPECT_DOUBLE_EQ(track.clips()[0].sourceIn().seconds(), 10.0);
    EXPECT_DOUBLE_EQ(track.clips()[1].sourceIn().seconds(), 31.5);
    EXPECT_DOUBLE_EQ(track.clips()[1].sourceOut().seconds(), 35.5);
}

TEST(EditorApiTest, AutomatedRippleDeleteReturnsStructuredResult) {
    auto project = makeProject();
    auto& track = project.timeline().ensureVideoTrack();
    track.addClip(makeClip(0.0, 5.0));
    track.addClip(makeClip(5.0, 3.0, 10.0));
    track.addClip(makeClip(8.0, 4.0, 20.0));

    const QJsonObject request{
        {QStringLiteral("op"), QStringLiteral("ripple_delete")},
        {QStringLiteral("trackIndex"), 0},
        {QStringLiteral("clipIndex"), 1}
    };
    const QJsonObject result = ccos::api::EditorApi::command(project, request);

    EXPECT_TRUE(result.value(QStringLiteral("ok")).toBool());
    EXPECT_EQ(result.value(QStringLiteral("remainingClips")).toInt(), 2);
    ASSERT_EQ(track.clips().size(), 2U);
    EXPECT_DOUBLE_EQ(track.clips()[1].start().seconds(), 5.0);
}

TEST(EditorApiTest, RejectsIntegerIndexOverflowWithoutUndefinedCast) {
    auto project = makeProject();
    const QJsonObject request{
        {QStringLiteral("op"), QStringLiteral("slip")},
        {QStringLiteral("trackIndex"), 1.0e30},
        {QStringLiteral("clipIndex"), 0},
        {QStringLiteral("sourceDeltaMs"), 0}
    };

    const QJsonObject result = ccos::api::EditorApi::command(project, request);
    EXPECT_FALSE(result.value(QStringLiteral("ok")).toBool());
    EXPECT_NE(result.value(QStringLiteral("error")).toString().indexOf(QStringLiteral("trackIndex")), -1);
}

TEST(EditorApiTest, ValidationRejectsInvalidClipTransform) {
    auto project = makeProject();
    ccos::media::MediaAsset asset(QStringLiteral("demo.mp4"));
    asset.metadata().durationMs = 10000;
    project.addAsset(asset);

    auto& track = project.timeline().ensureVideoTrack();
    auto clip = ccos::timeline::Clip(project.assets().front());
    clip.setDuration(ccos::core::Time::fromSeconds(2.0));
    clip.transform().opacity = 1.5;
    track.addClip(clip);

    const QJsonObject result = ccos::api::EditorApi::validate(project);
    EXPECT_FALSE(result.value(QStringLiteral("ok")).toBool());

    const auto errors = result.value(QStringLiteral("errors")).toArray();
    bool foundOpacityError = false;
    for (const auto& error : errors) {
        if (error.toString().contains(QStringLiteral("opacity outside"))) {
            foundOpacityError = true;
            break;
        }
    }
    EXPECT_TRUE(foundOpacityError);
}
