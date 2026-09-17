#pragma once

#include <cstddef>
#include <span>

namespace ccos::audio {

struct MeterReading {
    float peak = 0.0F;
    float rms = 0.0F;
    float peakDbfs = -100.0F;
    float rmsDbfs = -100.0F;
};

class AudioMeter final {
public:
    [[nodiscard]] static MeterReading analyze(std::span<const float> samples,
                                              std::size_t channels = 1) noexcept;
    [[nodiscard]] static float toDbfs(float linear) noexcept;
};

} // namespace ccos::audio
