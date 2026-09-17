#pragma once
#include "audio/AudioBuffer.hpp"
#include <algorithm>
#include <cmath>

namespace ccos::audio {
struct Fade { double start = 0.0; double end = 0.0; };

class AudioDsp {
public:
    static void applyGain(AudioBuffer& buffer, double gain) noexcept {
        for (float& sample : buffer.samples()) sample = static_cast<float>(sample * gain);
    }
    static void applyLinearFade(AudioBuffer& buffer, double fadeInSeconds, double fadeOutSeconds) noexcept {
        if (buffer.sampleRate() <= 0 || buffer.channels() <= 0) return;
        const qint64 frames = static_cast<qint64>(buffer.samples().size() / buffer.channels());
        const qint64 inFrames = static_cast<qint64>(std::max(0.0, fadeInSeconds) * buffer.sampleRate());
        const qint64 outFrames = static_cast<qint64>(std::max(0.0, fadeOutSeconds) * buffer.sampleRate());
        for (qint64 f = 0; f < frames; ++f) {
            double gain = 1.0;
            if (inFrames > 0 && f < inFrames) gain *= static_cast<double>(f) / inFrames;
            if (outFrames > 0 && f >= frames - outFrames) gain *= static_cast<double>(frames - f) / outFrames;
            for (int ch = 0; ch < buffer.channels(); ++ch) buffer.samples()[static_cast<std::size_t>(f * buffer.channels() + ch)] = static_cast<float>(buffer.samples()[static_cast<std::size_t>(f * buffer.channels() + ch)] * gain);
        }
    }
    static void softClip(AudioBuffer& buffer) noexcept {
        for (float& sample : buffer.samples()) sample = static_cast<float>(std::tanh(sample));
    }
};
}
