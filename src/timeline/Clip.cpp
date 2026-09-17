#include "timeline/Clip.hpp"

namespace ccos::timeline {
Clip::Clip() = default;
Clip::Clip(const ccos::media::MediaAsset& asset) : assetId_(asset.id()) {
    duration_ = ccos::core::Time::fromSeconds(static_cast<double>(asset.metadata().durationMs) / 1000.0);
    sourceIn_ = {};
    sourceOut_ = duration_;
}

void Clip::setSourceRange(ccos::core::Time in, ccos::core::Time out) noexcept {
    if (out < in) return;
    sourceIn_ = in;
    sourceOut_ = out;
    duration_ = out - in;
}
}
