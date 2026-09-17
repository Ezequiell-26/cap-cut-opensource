#include "timeline/TimelineEditor.hpp"
#include <gtest/gtest.h>

namespace {
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

TEST(TimelineEditorTests, SplitsClipAtTimelinePosition) {
    ccos::media::MediaAsset asset(QStringLiteral("demo.mp4"));
    asset.metadata().durationMs = 10000;
    ccos::timeline::Clip clip(asset);
    clip.setStart(ccos::core::Time::fromSeconds(2.0));
    const auto originalId = clip.id().toString();

    ccos::timeline::Track track(ccos::timeline::TrackType::Video);
    track.addClip(clip);
    ASSERT_TRUE(ccos::timeline::TimelineEditor::splitClip(track, 0, ccos::core::Time::fromSeconds(6.0)));
    ASSERT_EQ(track.clips().size(), 2U);
    EXPECT_DOUBLE_EQ(track.clips()[0].duration().seconds(), 4.0);
    EXPECT_DOUBLE_EQ(track.clips()[1].start().seconds(), 6.0);
    EXPECT_DOUBLE_EQ(track.clips()[1].duration().seconds(), 6.0);
    EXPECT_EQ(track.clips()[0].id().toString(), originalId);
    EXPECT_NE(track.clips()[1].id().toString(), originalId);
    EXPECT_NE(track.clips()[0].id(), track.clips()[1].id());
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

TEST(TimelineEditorTests, SlipMovesSourceWindowWithoutMovingTimelineWindow) {
    ccos::timeline::Track track(ccos::timeline::TrackType::Video);
    track.addClip(makeClip(10.0, 5.0, 20.0));

    ASSERT_TRUE(ccos::timeline::TimelineEditor::slipClip(track, 0, ccos::core::Time::fromSeconds(3.0)));
    EXPECT_DOUBLE_EQ(track.clips()[0].start().seconds(), 10.0);
    EXPECT_DOUBLE_EQ(track.clips()[0].duration().seconds(), 5.0);
    EXPECT_DOUBLE_EQ(track.clips()[0].sourceIn().seconds(), 23.0);
    EXPECT_DOUBLE_EQ(track.clips()[0].sourceOut().seconds(), 28.0);
}

TEST(TimelineEditorTests, SlipRejectsSourceRangeBeforeZero) {
    ccos::timeline::Track track(ccos::timeline::TrackType::Video);
    track.addClip(makeClip(10.0, 5.0, 2.0));

    EXPECT_FALSE(ccos::timeline::TimelineEditor::slipClip(track, 0, ccos::core::Time::fromSeconds(-3.0)));
    EXPECT_DOUBLE_EQ(track.clips()[0].sourceIn().seconds(), 2.0);
    EXPECT_DOUBLE_EQ(track.clips()[0].sourceOut().seconds(), 7.0);
}

TEST(TimelineEditorTests, RippleDeleteClosesGapOnlyForLaterClips) {
    ccos::timeline::Track track(ccos::timeline::TrackType::Video);
    track.addClip(makeClip(0.0, 5.0));
    track.addClip(makeClip(5.0, 3.0, 10.0));
    track.addClip(makeClip(8.0, 4.0, 20.0));

    ASSERT_TRUE(ccos::timeline::TimelineEditor::rippleDelete(track, 1));
    ASSERT_EQ(track.clips().size(), 2U);
    EXPECT_DOUBLE_EQ(track.clips()[0].start().seconds(), 0.0);
    EXPECT_DOUBLE_EQ(track.clips()[1].start().seconds(), 5.0);
    EXPECT_DOUBLE_EQ(track.clips()[1].duration().seconds(), 4.0);
    EXPECT_DOUBLE_EQ(track.clips()[1].sourceIn().seconds(), 20.0);
}
