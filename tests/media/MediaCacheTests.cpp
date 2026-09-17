#include "media/MediaCache.hpp"

#include <gtest/gtest.h>
#include <QFile>
#include <QTemporaryDir>

TEST(MediaCacheTests, WritesReadsAndRemovesVariants) {
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    ccos::media::MediaCache cache(dir.path(), 1024 * 1024);
    const QByteArray data("thumbnail-data");
    ASSERT_TRUE(cache.write(QStringLiteral("/media/a.mp4"), QStringLiteral("thumb-320"), data, QStringLiteral("bin")));
    EXPECT_EQ(cache.read(QStringLiteral("/media/a.mp4"), QStringLiteral("thumb-320"), QStringLiteral("bin")), data);
    EXPECT_TRUE(cache.remove(QStringLiteral("/media/a.mp4"), QStringLiteral("thumb-320"), QStringLiteral("bin")));
    EXPECT_TRUE(cache.read(QStringLiteral("/media/a.mp4"), QStringLiteral("thumb-320"), QStringLiteral("bin")).isEmpty());
}

TEST(MediaCacheTests, RejectsPathTraversalInExtension) {
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    ccos::media::MediaCache cache(dir.path());

    EXPECT_TRUE(cache.pathFor(QStringLiteral("source"), QStringLiteral("variant"), QStringLiteral("bin")).startsWith(dir.path()));
    EXPECT_TRUE(cache.pathFor(QStringLiteral("source"), QStringLiteral("variant"), QStringLiteral("../escape")).isEmpty());
    EXPECT_FALSE(cache.write(QStringLiteral("source"), QStringLiteral("variant"), QByteArrayLiteral("x"), QStringLiteral("../../escape")));
}

TEST(MediaCacheTests, ClearDoesNotDeleteUnrelatedFiles) {
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    ccos::media::MediaCache cache(dir.path());

    const QString unrelatedPath = dir.filePath(QStringLiteral("keep.txt"));
    QFile unrelated(unrelatedPath);
    ASSERT_TRUE(unrelated.open(QIODevice::WriteOnly));
    ASSERT_EQ(unrelated.write(QByteArrayLiteral("keep")), 4);
    unrelated.close();

    ASSERT_TRUE(cache.write(QStringLiteral("/media/a.mp4"), QStringLiteral("thumb-320"), QByteArrayLiteral("cache"), QStringLiteral("bin")));
    cache.clear();

    EXPECT_TRUE(QFile::exists(unrelatedPath));
    EXPECT_TRUE(cache.read(QStringLiteral("/media/a.mp4"), QStringLiteral("thumb-320"), QStringLiteral("bin")).isEmpty());
}

TEST(MediaCacheTests, ChangesSourceFingerprintWhenFileSizeChanges) {
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    ccos::media::MediaCache cache(dir.path());

    const QString source = dir.filePath(QStringLiteral("source.mp4"));
    QFile file(source);
    ASSERT_TRUE(file.open(QIODevice::WriteOnly));
    ASSERT_EQ(file.write(QByteArrayLiteral("first-version")), 13);
    file.close();

    const QString firstKey = cache.keyFor(source, QStringLiteral("thumbnail_320x180_0ms"));

    ASSERT_TRUE(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
    ASSERT_EQ(file.write(QByteArrayLiteral("a-different-second-version-with-more-bytes")), 42);
    file.close();

    const QString secondKey = cache.keyFor(source, QStringLiteral("thumbnail_320x180_0ms"));
    EXPECT_NE(firstKey, secondKey);
}
