#pragma once
#include "audio/AudioBuffer.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace ccos::audio {
struct Fade { double start = 0.0; double end = 0.0; };

class AudioDsp {
public:
    static void applyGain(AudioBuffer& buffer, double gain) noexcept {
        if (!std::isfinite(gain)) gain = 1.0;
        for (float& sample : buffer.samples()) sample = static_cast<float>(sample * gain);
    }
    static void applyLinearFade(AudioBuffer& buffer, double fadeInSeconds, double fadeOutSeconds) noexcept {
        if (buffer.sampleRate() <= 0 || buffer.channels() <= 0) return;
        const std::int64_t frames = static_cast<std::int64_t>(buffer.samples().size() / static_cast<std::size_t>(buffer.channels()));
        const std::int64_t inFrames = static_cast<std::int64_t>(std::max(0.0, fadeInSeconds) * buffer.sampleRate());
        const std::int64_t outFrames = static_cast<std::int64_t>(std::max(0.0, fadeOutSeconds) * buffer.sampleRate());
        for (std::int64_t f = 0; f < frames; ++f) {
            double gain = 1.0;
            if (inFrames > 0 && f < inFrames) gain *= static_cast<double>(f) / static_cast<double>(inFrames);
            if (outFrames > 0 && f >= frames - outFrames) gain *= static_cast<double>(frames - f) / static_cast<double>(outFrames);
            for (std::int32_t ch = 0; ch < buffer.channels(); ++ch) {
                const auto index = static_cast<std::size_t>(f * static_cast<std::int64_t>(buffer.channels()) + ch);
                buffer.samples()[index] = static_cast<float>(buffer.samples()[index] * gain);
            }
        }
    }
    static void softClip(AudioBuffer& buffer) noexcept {
        for (float& sample : buffer.samples()) sample = static_cast<float>(std::tanh(sample));
    }
};
}
