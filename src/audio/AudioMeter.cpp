#include "audio/AudioMeter.hpp"

#include <algorithm>
#include <cmath>

namespace ccos::audio {

float AudioMeter::toDbfs(float linear) noexcept {
    if (!(linear > 0.0F) || !std::isfinite(linear)) return -100.0F;
    return std::max(-100.0F, 20.0F * std::log10(linear));
}

MeterReading AudioMeter::analyze(std::span<const float> samples, std::size_t channels) noexcept {
    MeterReading reading;
    if (samples.empty() || channels == 0) return reading;

    long double sumSquares = 0.0L;
    float peak = 0.0F;
    std::size_t validSamples = 0;
    for (const float sample : samples) {
        if (!std::isfinite(sample)) continue;
        peak = std::max(peak, std::abs(sample));
        const long double value = static_cast<long double>(sample);
        sumSquares += value * value;
        ++validSamples;
    }
    if (validSamples == 0) return reading;

    reading.peak = peak;
    reading.rms = static_cast<float>(std::sqrt(sumSquares / static_cast<long double>(validSamples)));
    reading.peakDbfs = toDbfs(reading.peak);
    reading.rmsDbfs = toDbfs(reading.rms);
    return reading;
}

} // namespace ccos::audio
