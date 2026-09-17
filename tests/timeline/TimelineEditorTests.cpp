#include "timeline/TimelineEditor.hpp"
#include <gtest/gtest.h>

TEST(TimelineEditorTests, SplitsClipAtTimelinePosition) {
    ccos::media::MediaAsset asset(QStringLiteral("demo.mp4"));
    asset.metadata().durationMs = 10000;
    ccos::timeline::Clip clip(asset);
    clip.setStart(ccos::core::Time::fromSeconds(2.0));

    ccos::timeline::Track track(ccos::timeline::TrackType::Video);
    track.addClip(clip);
    ASSERT_TRUE(ccos::timeline::TimelineEditor::splitClip(track, 0, ccos::core::Time::fromSeconds(6.0)));
    ASSERT_EQ(track.clips().size(), 2U);
    EXPECT_DOUBLE_EQ(track.clips()[0].duration().seconds(), 4.0);
    EXPECT_DOUBLE_EQ(track.clips()[1].start().seconds(), 6.0);
    EXPECT_DOUBLE_EQ(track.clips()[1].duration().seconds(), 6.0);
}

TEST(TimelineEditorTests, TrimsSourceRange) {
    ccos::media::MediaAsset asset(QStringLiteral("demo.mp4"));
    asset.metadata().durationMs = 10000;
    ccos::timeline::Clip clip(asset);
    ccos::timeline::Track track(ccos::timeline::TrackType::Video);
    track.addClip(clip);
    ASSERT_TRUE(ccos::timeline::TimelineEditor::trimClip(track, 0, ccos::core::Time::fromSeconds(1.0), ccos::core::Time::fromSeconds(7.0)));
    EXPECT_DOUBLE_EQ(track.clips()[0].sourceIn().seconds(), 1.0);
    EXPECT_DOUBLE_EQ(track.clips()[0].sourceOut().seconds(), 7.0);
    EXPECT_DOUBLE_EQ(track.clips()[0].duration().seconds(), 6.0);
}
