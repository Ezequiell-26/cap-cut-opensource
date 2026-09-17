#include "timeline/Timeline.hpp"
#include <gtest/gtest.h>

TEST(TimelineTests, StartsWithVideoAndAudioTracks) {
    ccos::timeline::Timeline timeline;
    ASSERT_EQ(timeline.tracks().size(), 2U);
    EXPECT_EQ(timeline.tracks()[0].type(), ccos::timeline::TrackType::Video);
    EXPECT_EQ(timeline.tracks()[1].type(), ccos::timeline::TrackType::Audio);
}

TEST(TimelineTests, AddsClipToVideoTrack) {
    ccos::media::MediaAsset asset(QStringLiteral("demo.mp4"));
    asset.metadata().durationMs = 2500;
    ccos::timeline::Clip clip(asset);
    ccos::timeline::Timeline timeline;
    timeline.addClipToVideo(clip);
    ASSERT_EQ(timeline.tracks()[0].clips().size(), 1U);
    EXPECT_DOUBLE_EQ(timeline.tracks()[0].clips()[0].duration().seconds(), 2.5);
}
