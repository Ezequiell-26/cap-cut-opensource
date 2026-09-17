#pragma once
#include "timeline/Timeline.hpp"

namespace ccos::timeline {
class TimelineEditor {
public:
    static bool trimClip(Track& track, std::size_t clipIndex, ccos::core::Time newIn, ccos::core::Time newOut);
    static bool splitClip(Track& track, std::size_t clipIndex, ccos::core::Time timelineOffset);
    static bool moveClip(Track& track, std::size_t clipIndex, ccos::core::Time newStart);
    static bool slipClip(Track& track, std::size_t clipIndex, ccos::core::Time sourceDelta);
    static bool deleteClip(Track& track, std::size_t clipIndex);
    static bool rippleDelete(Track& track, std::size_t clipIndex);
};
}
