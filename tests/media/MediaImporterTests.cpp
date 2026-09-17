#include "media/MediaImporter.hpp"

#include <gtest/gtest.h>

TEST(MediaImporterTests, EmptyPathsDoNotInvokeProbe) {
    const QStringList paths{QString(), QStringLiteral("  ")};
    QString error;
    const auto assets = ccos::media::MediaImporter::importFiles(paths, 10, &error);

    EXPECT_TRUE(assets.empty());
    EXPECT_TRUE(error.isEmpty());
}

TEST(MediaImporterTests, RejectsBatchAboveConfiguredLimit) {
    const QStringList paths{
        QStringLiteral("one.mp4"), QStringLiteral("two.mp4"), QStringLiteral("three.mp4")};
    QString error;
    const auto assets = ccos::media::MediaImporter::importFiles(paths, 2, &error);

    EXPECT_TRUE(assets.empty());
    EXPECT_NE(error.indexOf(QStringLiteral("limit is 2")), -1);
}

TEST(MediaImporterTests, ZeroLimitFailsClosed) {
    QString error;
    const auto assets = ccos::media::MediaImporter::importFiles(
        {QStringLiteral("one.mp4")}, 0, &error);

    EXPECT_TRUE(assets.empty());
    EXPECT_EQ(error, QStringLiteral("Maximum import file count must be greater than zero"));
}
