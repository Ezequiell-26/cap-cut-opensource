#include "core/Time.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <sstream>

namespace ccos::core {

namespace {
constexpr std::int64_t gcd64(std::int64_t a, std::int64_t b) noexcept {
    a = a < 0 ? -a : a;
    b = b < 0 ? -b : b;
    while (b != 0) {
        const auto r = a % b;
        a = b;
        b = r;
    }
    return a == 0 ? 1 : a;
}
}

constexpr Time::Time(std::int64_t numerator, std::int32_t denominator) noexcept
    : numerator_(numerator), denominator_(denominator <= 0 ? 1 : denominator) {
    normalize();
}

double Time::seconds() const noexcept {
    return static_cast<double>(numerator_) / static_cast<double>(denominator_);
}

std::string Time::toString() const {
    std::ostringstream out;
    out << numerator_ << '/' << denominator_;
    return out.str();
}

void Time::normalize() noexcept {
    if (denominator_ < 0) {
        denominator_ = -denominator_;
        numerator_ = -numerator_;
    }
    const auto divisor = gcd64(numerator_, denominator_);
    numerator_ /= divisor;
    denominator_ = static_cast<std::int32_t>(denominator_ / divisor);
}

constexpr Time operator+(Time lhs, Time rhs) noexcept {
    return Time(lhs.numerator_ * rhs.denominator_ + rhs.numerator_ * lhs.denominator_,
                static_cast<std::int32_t>(lhs.denominator_ * rhs.denominator_));
}

constexpr Time operator-(Time lhs, Time rhs) noexcept {
    return Time(lhs.numerator_ * rhs.denominator_ - rhs.numerator_ * lhs.denominator_,
                static_cast<std::int32_t>(lhs.denominator_ * rhs.denominator_));
}

constexpr bool operator==(Time lhs, Time rhs) noexcept {
    return lhs.numerator_ * rhs.denominator_ == rhs.numerator_ * lhs.denominator_;
}

constexpr auto operator<=>(Time lhs, Time rhs) noexcept {
    const auto left = lhs.numerator_ * static_cast<std::int64_t>(rhs.denominator_);
    const auto right = rhs.numerator_ * static_cast<std::int64_t>(lhs.denominator_);
    return left <=> right;
}

constexpr Time Time::fromSeconds(double value, std::int32_t denominator) noexcept {
    if (!std::isfinite(value) || denominator <= 0) {
        return {};
    }
    return Time(static_cast<std::int64_t>(std::llround(value * denominator)), denominator);
}

constexpr Time Time::fromFrames(std::int64_t frame, std::int32_t fpsNumerator, std::int32_t fpsDenominator) noexcept {
    if (fpsNumerator <= 0 || fpsDenominator <= 0) {
        return {};
    }
    return Time(frame * static_cast<std::int64_t>(fpsDenominator), fpsNumerator);
}

} // namespace ccos::core
