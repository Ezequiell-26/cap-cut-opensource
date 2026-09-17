#include "timeline/Timeline.hpp"
#include <algorithm>

namespace ccos::timeline {
Timeline::Timeline() { ensureVideoTrack(); ensureAudioTrack(); }
Track& Timeline::ensureVideoTrack() {
    const auto it = std::find_if(tracks_.begin(), tracks_.end(), [](const Track& t) { return t.type() == TrackType::Video; });
    if (it != tracks_.end()) return *it;
    tracks_.emplace_back(TrackType::Video);
    return tracks_.back();
}
Track& Timeline::ensureAudioTrack() {
    const auto it = std::find_if(tracks_.begin(), tracks_.end(), [](const Track& t) { return t.type() == TrackType::Audio; });
    if (it != tracks_.end()) return *it;
    tracks_.emplace_back(TrackType::Audio);
    return tracks_.back();
}
void Timeline::addClipToVideo(const Clip& clip) { ensureVideoTrack().addClip(clip); }
}
