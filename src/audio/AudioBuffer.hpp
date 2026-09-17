#pragma once
#include <cstdint>
#include <vector>

namespace ccos::audio {
class AudioBuffer {
public:
    AudioBuffer() = default;
    AudioBuffer(std::int32_t channels, std::int32_t frames) : channels_(channels), frames_(frames), samples_(static_cast<std::size_t>(channels) * static_cast<std::size_t>(frames), 0.0f) {}
    [[nodiscard]] std::int32_t channels() const noexcept { return channels_; }
    [[nodiscard]] std::int32_t frames() const noexcept { return frames_; }
    [[nodiscard]] float* data() noexcept { return samples_.data(); }
    [[nodiscard]] const float* data() const noexcept { return samples_.data(); }
private:
    std::int32_t channels_ = 0;
    std::int32_t frames_ = 0;
    std::vector<float> samples_;
};
}
