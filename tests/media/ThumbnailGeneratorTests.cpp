#include "media/ThumbnailGenerator.hpp"
#include "media/MediaCache.hpp"

#include <gtest/gtest.h>

#include <QTemporaryDir>
#include <QStringList>

namespace {

ccos::media::MediaAsset makeAsset() {
    ccos::core::Uuid id;
    ccos::media::MediaAsset asset(id, QStringLiteral("/media/source.mp4"));
    asset.setName(QStringLiteral("Source"));
    return asset;
}

} // namespace

TEST(ThumbnailGeneratorTests, BuildsBoundedSeekAndScaleArguments) {
    const auto asset = makeAsset();
    QString error;
    const QStringList args = ccos::media::ThumbnailGenerator::buildArguments(
        asset, QStringLiteral("/cache/thumb.jpg"), 1500, 320, 180, &error);

    ASSERT_FALSE(args.isEmpty()) << error.toStdString();
    EXPECT_EQ(args.at(0), QStringLiteral("-hide_banner"));
    EXPECT_EQ(args.at(args.indexOf(QStringLiteral("-ss")) + 1), QStringLiteral("1.500"));
    EXPECT_EQ(args.at(args.indexOf(QStringLiteral("-frames:v")) + 1), QStringLiteral("1"));
    EXPECT_EQ(args.last(), QStringLiteral("/cache/thumb.jpg"));
    EXPECT_FALSE(args.contains(QStringLiteral("-filter_complex")));
}

TEST(ThumbnailGeneratorTests, RejectsNegativeSeek) {
    const auto asset = makeAsset();
    QString error;
    EXPECT_TRUE(ccos::media::ThumbnailGenerator::buildArguments(
        asset, QStringLiteral("/cache/thumb.jpg"), -1, 320, 180, &error).isEmpty());
    EXPECT_NE(error.indexOf(QStringLiteral("position")), -1);
}

TEST(ThumbnailGeneratorTests, RejectsOversizedDimensions) {
    const auto asset = makeAsset();
    QString error;
    EXPECT_TRUE(ccos::media::ThumbnailGenerator::buildArguments(
        asset, QStringLiteral("/cache/thumb.jpg"), 0, 8192, 180, &error).isEmpty());
    EXPECT_NE(error.indexOf(QStringLiteral("dimensions")), -1);
}

TEST(ThumbnailGeneratorTests, RejectsSourceOutputCollision) {
    const auto asset = makeAsset();
    QString error;
    EXPECT_TRUE(ccos::media::ThumbnailGenerator::buildArguments(
        asset, QStringLiteral("/media/source.mp4"), 0, 320, 180, &error).isEmpty());
    EXPECT_NE(error.indexOf(QStringLiteral("overwrite")), -1);
}

TEST(ThumbnailGeneratorTests, GeneratesStableCachePathsForSameRequest) {
    const auto asset = makeAsset();
    QTemporaryDir tempDir;
    ASSERT_TRUE(tempDir.isValid());
    ccos::media::MediaCache cache(tempDir.path());

    const QString first = ccos::media::ThumbnailGenerator::thumbnailPath(cache, asset, 250, 320, 180);
    const QString second = ccos::media::ThumbnailGenerator::thumbnailPath(cache, asset, 250, 320, 180);
    const QString different = ccos::media::ThumbnailGenerator::thumbnailPath(cache, asset, 500, 320, 180);

    EXPECT_FALSE(first.isEmpty());
    EXPECT_EQ(first, second);
    EXPECT_NE(first, different);
    EXPECT_TRUE(first.endsWith(QStringLiteral(".jpg")));
    EXPECT_TRUE(first.startsWith(tempDir.path()));
}
