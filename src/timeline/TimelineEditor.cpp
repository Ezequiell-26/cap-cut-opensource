#include "timeline/TimelineEditor.hpp"
#include <algorithm>

namespace ccos::timeline {

bool TimelineEditor::trimClip(Track& track, std::size_t clipIndex, ccos::core::Time newIn, ccos::core::Time newOut) {
    if (clipIndex >= track.clips().size() || newOut < newIn) return false;
    auto& clip = track.clips()[clipIndex];
    if (newOut == newIn) return false;
    const auto oldSourceOut = clip.sourceOut();
    if (newIn < clip.sourceIn() || newOut > oldSourceOut) return false;
    clip.setSourceRange(newIn, newOut);
    return true;
}

bool TimelineEditor::splitClip(Track& track, std::size_t clipIndex, ccos::core::Time timelineOffset) {
    if (clipIndex >= track.clips().size()) return false;
    const auto original = track.clips()[clipIndex];
    if (timelineOffset <= original.start() || timelineOffset >= original.start() + original.duration()) return false;

    const auto leftDuration = timelineOffset - original.start();
    const auto rightDuration = original.duration() - leftDuration;
    auto left = original;
    auto right = original;
    left.setDuration(leftDuration);
    right.setStart(timelineOffset);
    right.setSourceRange(original.sourceIn() + leftDuration, original.sourceOut());
    track.clips()[clipIndex] = left;
    track.clips().insert(track.clips().begin() + static_cast<std::ptrdiff_t>(clipIndex) + 1, right);
    return true;
}

bool TimelineEditor::moveClip(Track& track, std::size_t clipIndex, ccos::core::Time newStart) {
    if (clipIndex >= track.clips().size() || newStart < ccos::core::Time{}) return false;
    track.clips()[clipIndex].setStart(newStart);
    return true;
}

bool TimelineEditor::deleteClip(Track& track, std::size_t clipIndex) {
    if (clipIndex >= track.clips().size()) return false;
    track.clips().erase(track.clips().begin() + static_cast<std::ptrdiff_t>(clipIndex));
    return true;
}

}
