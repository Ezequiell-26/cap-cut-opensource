#include "timeline/Track.hpp"

namespace ccos::timeline {
Track::Track(TrackType type) : Track(type, type == TrackType::Video ? QStringLiteral("Video 1") : QStringLiteral("Audio 1")) {}
Track::Track(TrackType type, QString name) : type_(type), name_(std::move(name)) {}
}
