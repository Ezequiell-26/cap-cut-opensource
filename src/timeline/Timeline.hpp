#pragma once
#include "timeline/Track.hpp"
#include <vector>

namespace ccos::timeline {
class Timeline {
public:
    Timeline();
    [[nodiscard]] const std::vector<Track>& tracks() const noexcept { return tracks_; }
    std::vector<Track>& tracks() noexcept { return tracks_; }
    Track& ensureVideoTrack();
    Track& ensureAudioTrack();
    void addClipToVideo(const Clip& clip);
private:
    std::vector<Track> tracks_;
};
}
