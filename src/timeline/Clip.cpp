#include "timeline/Clip.hpp"

namespace ccos::timeline {
Clip::Clip() = default;
Clip::Clip(const ccos::media::MediaAsset& asset) : assetId_(asset.id()) {
    duration_ = ccos::core::Time::fromSeconds(static_cast<double>(asset.metadata().durationMs) / 1000.0);
}
}
