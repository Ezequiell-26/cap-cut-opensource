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
    const auto originalEnd = original.start() + original.duration();
    if (timelineOffset <= original.start() || timelineOffset >= originalEnd) return false;

    const auto leftDuration = timelineOffset - original.start();
    const auto rightDuration = original.duration() - leftDuration;
    auto left = original;
    auto right = original;

    // A split creates a new entity for the right-hand fragment. The left half
    // preserves the original ID so external references remain stable.
    right.regenerateId();
    left.setDuration(leftDuration);
    right.setStart(timelineOffset);
    right.setDuration(rightDuration);
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

bool TimelineEditor::slipClip(Track& track, std::size_t clipIndex, ccos::core::Time sourceDelta) {
    if (clipIndex >= track.clips().size()) return false;
    auto& clip = track.clips()[clipIndex];

    const auto newIn = clip.sourceIn() + sourceDelta;
    const auto newOut = clip.sourceOut() + sourceDelta;
    const auto duration = clip.duration();
    if (newIn < ccos::core::Time{} || newOut <= newIn || (newOut - newIn) != duration) return false;

    clip.setSourceRange(newIn, newOut);
    return true;
}

bool TimelineEditor::deleteClip(Track& track, std::size_t clipIndex) {
    if (clipIndex >= track.clips().size()) return false;
    track.clips().erase(track.clips().begin() + static_cast<std::ptrdiff_t>(clipIndex));
    return true;
}

bool TimelineEditor::rippleDelete(Track& track, std::size_t clipIndex) {
    if (clipIndex >= track.clips().size()) return false;

    const auto removedStart = track.clips()[clipIndex].start();
    const auto removedDuration = track.clips()[clipIndex].duration();
    if (removedDuration <= ccos::core::Time{}) return deleteClip(track, clipIndex);

    const auto removedEnd = removedStart + removedDuration;
    track.clips().erase(track.clips().begin() + static_cast<std::ptrdiff_t>(clipIndex));

    for (auto& clip : track.clips()) {
        if (clip.start() >= removedEnd) clip.setStart(clip.start() - removedDuration);
    }
    return true;
}

}
