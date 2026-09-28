#include "project/Project.hpp"
#include "project/ProjectSerializer.hpp"
#include <gtest/gtest.h>
#include <QTemporaryDir>

TEST(ProjectClipPropertiesTests, PersistsTransformSpeedAndEffects) {
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());

    ccos::project::Project source(QStringLiteral("Properties"));
    ccos::media::MediaAsset asset(QStringLiteral("/tmp/demo.mp4"));
    source.addAsset(asset);
    auto& track = source.timeline().ensureVideoTrack();
    ccos::timeline::Clip clip(asset);
    clip.setSpeed(1.5);
    clip.transform().x = 120.0;
    clip.transform().y = 30.0;
    clip.transform().scaleX = 1.25;
    clip.transform().opacity = 0.75;
    clip.addEffect(QStringLiteral("grayscale"));
    track.addClip(clip);

    const QString path = dir.filePath(QStringLiteral("properties.ccos"));
    QString error;
    ASSERT_TRUE(ccos::project::ProjectSerializer::save(source, path, &error)) << error.toStdString();

    ccos::project::Project loaded;
    ASSERT_TRUE(ccos::project::ProjectSerializer::load(loaded, path, &error)) << error.toStdString();
    ASSERT_EQ(loaded.timeline().tracks().size(), 2U);
    const auto& loadedClip = loaded.timeline().tracks().front().clips().front();
    EXPECT_DOUBLE_EQ(loadedClip.speed(), 1.5);
    EXPECT_DOUBLE_EQ(loadedClip.transform().x, 120.0);
    EXPECT_DOUBLE_EQ(loadedClip.transform().y, 30.0);
    EXPECT_DOUBLE_EQ(loadedClip.transform().scaleX, 1.25);
    EXPECT_DOUBLE_EQ(loadedClip.transform().opacity, 0.75);
    ASSERT_EQ(loadedClip.effects().size(), 1);
    EXPECT_EQ(loadedClip.effects().front(), QStringLiteral("grayscale"));
}


TEST(ProjectClipPropertiesTests, PersistsAudioMixDefaults) {
    ccos::media::MediaAsset asset(QStringLiteral("/tmp/audio.mp4"));
    ccos::timeline::Clip clip(asset);
    EXPECT_DOUBLE_EQ(clip.audioGain(), 1.0);
    EXPECT_FALSE(clip.audioMuted());

    clip.setAudioGain(99.0);
    clip.setAudioMuted(true);
    EXPECT_DOUBLE_EQ(clip.audioGain(), 4.0);
    EXPECT_TRUE(clip.audioMuted());
}
