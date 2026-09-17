#pragma once
#include "audio/AudioBuffer.hpp"
#include <algorithm>
#include <cmath>

namespace ccos::audio {
class AudioMixer {
public:
    static void applyGain(AudioBuffer& buffer, float gain) noexcept {
        if (!std::isfinite(gain)) gain = 1.0f;
        float* samples = buffer.data();
        const auto count = static_cast<std::size_t>(buffer.channels()) * static_cast<std::size_t>(buffer.frames());
        for (std::size_t i = 0; i < count; ++i) samples[i] = std::clamp(samples[i] * gain, -1.0f, 1.0f);
    }
};
}
