#pragma once

#include <cstdint>
#include <compare>
#include <string>

namespace ccos::core {

class Time {
public:
    constexpr Time() noexcept = default;
    constexpr Time(std::int64_t numerator, std::int32_t denominator = 1) noexcept;

    [[nodiscard]] constexpr std::int64_t numerator() const noexcept { return numerator_; }
    [[nodiscard]] constexpr std::int32_t denominator() const noexcept { return denominator_; }
    [[nodiscard]] double seconds() const noexcept;
    [[nodiscard]] std::string toString() const;

    friend constexpr Time operator+(Time lhs, Time rhs) noexcept;
    friend constexpr Time operator-(Time lhs, Time rhs) noexcept;
    friend constexpr Time operator-(Time value) noexcept { return Time(-value.numerator_, value.denominator_); }
    friend constexpr bool operator==(Time lhs, Time rhs) noexcept;
    friend constexpr auto operator<=>(Time lhs, Time rhs) noexcept;

    static constexpr Time fromSeconds(double value, std::int32_t denominator = 1000000) noexcept;
    static constexpr Time fromFrames(std::int64_t frame, std::int32_t fpsNumerator, std::int32_t fpsDenominator = 1) noexcept;

private:
    std::int64_t numerator_ = 0;
    std::int32_t denominator_ = 1;
    void normalize() noexcept;
};

} // namespace ccos::core
