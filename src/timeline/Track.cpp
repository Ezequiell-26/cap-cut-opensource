#include "timeline/Track.hpp"
namespace ccos::timeline {
Track::Track(TrackType type) : type_(type), name_(type == TrackType::Video ? QStringLiteral("Video 1") : QStringLiteral("Audio 1")) {}
}
