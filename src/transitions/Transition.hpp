#pragma once
#include <QString>
#include <QVariantMap>

namespace ccos::transitions {
enum class TransitionType { Cut, Fade, Dissolve, DipToBlack, Wipe, Slide, Zoom };

class Transition {
public:
    explicit Transition(TransitionType type = TransitionType::Dissolve, qint64 durationMs = 500)
        : type_(type), durationMs_(durationMs) {}
    [[nodiscard]] TransitionType type() const noexcept { return type_; }
    [[nodiscard]] qint64 durationMs() const noexcept { return durationMs_; }
    void setDurationMs(qint64 value) noexcept { durationMs_ = value < 0 ? 0 : value; }
    [[nodiscard]] QString ffmpegName() const;
    [[nodiscard]] QString ffmpegFilter(double durationSeconds, double offsetSeconds) const;
private:
    TransitionType type_;
    qint64 durationMs_;
};
}
