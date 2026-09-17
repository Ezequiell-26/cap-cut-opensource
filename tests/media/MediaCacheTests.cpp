#include "media/MediaCache.hpp"
#include <gtest/gtest.h>
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
