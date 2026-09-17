#include "media/MediaProbe.hpp"

#include <gtest/gtest.h>

TEST(MediaProbeTests, EmptyMediaPathFailsBeforeLaunchingProcess) {
    ccos::media::MediaAsset asset(QString());
    QString error;

    EXPECT_FALSE(ccos::media::MediaProbe::probe(asset, QStringLiteral("ffprobe"), &error));
    EXPECT_EQ(error, QStringLiteral("Media path is empty"));
}
