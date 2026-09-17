#pragma once
#include "timeline/Keyframe.hpp"

namespace ccos::video {
struct Transform {
    ccos::timeline::KeyframeTrack x;
    ccos::timeline::KeyframeTrack y;
    ccos::timeline::KeyframeTrack scaleX;
    ccos::timeline::KeyframeTrack scaleY;
    ccos::timeline::KeyframeTrack rotation;
    ccos::timeline::KeyframeTrack opacity;

    [[nodiscard]] double positionX(ccos::core::Time time) const noexcept { return x.keyframes().empty() ? 0.0 : x.evaluate(time); }
    [[nodiscard]] double positionY(ccos::core::Time time) const noexcept { return y.keyframes().empty() ? 0.0 : y.evaluate(time); }
    [[nodiscard]] double currentScaleX(ccos::core::Time time) const noexcept { return scaleX.keyframes().empty() ? 1.0 : scaleX.evaluate(time); }
    [[nodiscard]] double currentScaleY(ccos::core::Time time) const noexcept { return scaleY.keyframes().empty() ? 1.0 : scaleY.evaluate(time); }
    [[nodiscard]] double currentRotation(ccos::core::Time time) const noexcept { return rotation.keyframes().empty() ? 0.0 : rotation.evaluate(time); }
    [[nodiscard]] double currentOpacity(ccos::core::Time time) const noexcept { return opacity.keyframes().empty() ? 1.0 : opacity.evaluate(time); }
};
}
