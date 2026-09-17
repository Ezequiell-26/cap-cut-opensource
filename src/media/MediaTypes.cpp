#include "media/MediaTypes.hpp"

namespace ccos::media {

// Implementación de MediaAsset
MediaAsset::MediaAsset(const Id& id, const std::filesystem::path& path)
    : id_(id), path_(path) {}

void MediaAsset::setMetadata(const MediaMetadata& metadata) {
    metadata_ = metadata;
    isValid_ = true;
    errorMessage_.clear();
}

void MediaAsset::addStream(const MediaStream& stream) {
    streams_.push_back(stream);
}

void MediaAsset::invalidate(const std::string& reason) {
    isValid_ = false;
    errorMessage_ = reason;
}

std::optional<MediaStream> MediaAsset::getVideoStream() const {
    for (const auto& stream : streams_) {
        if (stream.type == StreamType::VIDEO) {
            return stream;
        }
    }
    return std::nullopt;
}

std::optional<MediaStream> MediaAsset::getAudioStream() const {
    for (const auto& stream : streams_) {
        if (stream.type == StreamType::AUDIO) {
            return stream;
        }
    }
    return std::nullopt;
}

} // namespace ccos::media
