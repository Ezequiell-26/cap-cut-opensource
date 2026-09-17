#include "timeline/Keyframe.hpp"
#include <cmath>

namespace ccos::timeline {
void KeyframeTrack::add(Keyframe keyframe) {
    keyframes_.push_back(keyframe);
    std::sort(keyframes_.begin(), keyframes_.end(), [](const auto& a, const auto& b) { return a.time < b.time; });
}

double KeyframeTrack::evaluate(ccos::core::Time time) const noexcept {
    if (keyframes_.empty()) return 0.0;
    if (time <= keyframes_.front().time) return keyframes_.front().value;
    if (time >= keyframes_.back().time) return keyframes_.back().value;
    for (std::size_t i = 1; i < keyframes_.size(); ++i) {
        const auto& a = keyframes_[i - 1];
        const auto& b = keyframes_[i];
        if (time <= b.time) {
            const double span = (b.time - a.time).seconds();
            double t = span <= 0.0 ? 0.0 : (time - a.time).seconds() / span;
            switch (a.interpolation) {
                case Interpolation::EaseIn: t *= t; break;
                case Interpolation::EaseOut: t = 1.0 - (1.0 - t) * (1.0 - t); break;
                case Interpolation::EaseInOut: t = t < 0.5 ? 2.0 * t * t : 1.0 - std::pow(-2.0 * t + 2.0, 2.0) / 2.0; break;
                case Interpolation::Linear: break;
            }
            return a.value + (b.value - a.value) * t;
        }
    }
    return keyframes_.back().value;
}
}
