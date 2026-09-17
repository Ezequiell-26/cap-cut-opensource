#pragma once
#include "core/Time.hpp"
#include <algorithm>
#include <vector>

namespace ccos::timeline {
enum class Interpolation { Linear, EaseIn, EaseOut, EaseInOut };

struct Keyframe {
    ccos::core::Time time;
    double value = 0.0;
    Interpolation interpolation = Interpolation::Linear;
};

class KeyframeTrack {
public:
    void add(Keyframe keyframe);
    [[nodiscard]] double evaluate(ccos::core::Time time) const noexcept;
    [[nodiscard]] const std::vector<Keyframe>& keyframes() const noexcept { return keyframes_; }
private:
    std::vector<Keyframe> keyframes_;
};
}
