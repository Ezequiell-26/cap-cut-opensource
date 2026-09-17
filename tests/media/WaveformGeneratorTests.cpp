#include "media/WaveformGenerator.hpp"
#include "media/MediaCache.hpp"

#include <gtest/gtest.h>

#include <QTemporaryDir>

namespace {
ccos::media::MediaAsset makeAsset() {
    ccos::core::Uuid id;
    ccos::media::MediaAsset asset(id, QStringLiteral("/media/audio.wav"));
    asset.setName(QStringLiteral("Audio"));
    return asset;
}
}

TEST(WaveformGeneratorTests, BuildsSafeShowWavesPipeline) {
    const auto asset = makeAsset();
    QString error;
    const auto args = ccos::media::WaveformGenerator::buildArguments(
        asset, QStringLiteral("/cache/waveform.png"), 1200, 160, 100, &error);

    ASSERT_FALSE(args.isEmpty()) << error.toStdString();
    const int filterIndex = args.indexOf(QStringLiteral("-filter_complex"));
    ASSERT_GE(filterIndex, 0);
    EXPECT_NE(args.at(filterIndex + 1).indexOf(QStringLiteral("showwavespic")), -1);
    EXPECT_NE(args.at(filterIndex + 1).indexOf(QStringLiteral("aresample=100")), -1);
    EXPECT_EQ(args.last(), QStringLiteral("/cache/waveform.png"));
}

TEST(WaveformGeneratorTests, RejectsInvalidDensityAndDimensions) {
    const auto asset = makeAsset();
    QString error;
    EXPECT_TRUE(ccos::media::WaveformGenerator::buildArguments(
        asset, QStringLiteral("/cache/waveform.png"), 16, 160, 100, &error).isEmpty());
    EXPECT_FALSE(error.isEmpty());

    EXPECT_TRUE(ccos::media::WaveformGenerator::buildArguments(
        asset, QStringLiteral("/cache/waveform.png"), 1200, 160, 5000, &error).isEmpty());
    EXPECT_NE(error.indexOf(QStringLiteral("sample")), -1);
}

TEST(WaveformGeneratorTests, RejectsSourceOutputCollision) {
    const auto asset = makeAsset();
    QString error;
    EXPECT_TRUE(ccos::media::WaveformGenerator::buildArguments(
        asset, QStringLiteral("/media/audio.wav"), 1200, 160, 100, &error).isEmpty());
    EXPECT_NE(error.indexOf(QStringLiteral("overwrite")), -1);
}

TEST(WaveformGeneratorTests, CachePathChangesWithSourceFingerprint) {
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    QTemporaryDir cacheDir;
    ASSERT_TRUE(cacheDir.isValid());

    const QString source = dir.filePath(QStringLiteral("audio.wav"));
    QFile file(source);
    ASSERT_TRUE(file.open(QIODevice::WriteOnly));
    ASSERT_EQ(file.write(QByteArrayLiteral("short")), 5);
    file.close();

    ccos::media::MediaAsset asset(ccos::core::Uuid(), source);
    ccos::media::MediaCache cache(cacheDir.path());
    const QString first = ccos::media::WaveformGenerator::waveformPath(cache, asset);

    ASSERT_TRUE(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
    ASSERT_EQ(file.write(QByteArrayLiteral("a-longer-audio-fixture")), 21);
    file.close();

    const QString second = ccos::media::WaveformGenerator::waveformPath(cache, asset);
    EXPECT_NE(first, second);
}
