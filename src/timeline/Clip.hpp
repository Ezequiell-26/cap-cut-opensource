#pragma once
#include "core/Time.hpp"
#include "core/Uuid.hpp"
#include "media/MediaAsset.hpp"
#include <algorithm>
#include <cmath>
#include <QStringList>

namespace ccos::timeline {

struct TransformState {
    double x = 0.0;
    double y = 0.0;
    double scaleX = 1.0;
    double scaleY = 1.0;
    double rotation = 0.0;
    double opacity = 1.0;
    double cropLeft = 0.0;
    double cropTop = 0.0;
    double cropRight = 0.0;
    double cropBottom = 0.0;
    bool flipHorizontal = false;
    bool flipVertical = false;
};

class Clip {
public:
    Clip();
    explicit Clip(const ccos::media::MediaAsset& asset);
    Clip(ccos::core::Uuid id, ccos::core::Uuid assetId);
    [[nodiscard]] const ccos::core::Uuid& id() const noexcept { return id_; }
    [[nodiscard]] const ccos::core::Uuid& assetId() const noexcept { return assetId_; }
    [[nodiscard]] ccos::core::Time start() const noexcept { return start_; }
    [[nodiscard]] ccos::core::Time duration() const noexcept { return duration_; }
    [[nodiscard]] ccos::core::Time sourceIn() const noexcept { return sourceIn_; }
    [[nodiscard]] ccos::core::Time sourceOut() const noexcept { return sourceOut_; }
    [[nodiscard]] double speed() const noexcept { return speed_; }
    [[nodiscard]] double audioGain() const noexcept { return audioGain_; }
    [[nodiscard]] bool audioMuted() const noexcept { return audioMuted_; }
    [[nodiscard]] const TransformState& transform() const noexcept { return transform_; }
    TransformState& transform() noexcept { return transform_; }
    [[nodiscard]] const QStringList& effects() const noexcept { return effects_; }
    QStringList& effects() noexcept { return effects_; }
    [[nodiscard]] const QString& transitionInId() const noexcept { return transitionInId_; }
    [[nodiscard]] qint64 transitionInDurationMs() const noexcept { return transitionInDurationMs_; }
    void setTransitionIn(QString id, qint64 durationMs) { transitionInId_ = std::move(id); transitionInDurationMs_ = std::max<qint64>(0, durationMs); }
    void setSpeed(double value) noexcept { speed_ = value > 0.0 ? value : 1.0; }
    void setAudioGain(double value) noexcept { audioGain_ = std::isfinite(value) ? std::clamp(value, 0.0, 4.0) : 1.0; }
    void setAudioMuted(bool value) noexcept { audioMuted_ = value; }
    void setStart(ccos::core::Time value) noexcept { start_ = value; }
    void setDuration(ccos::core::Time value) noexcept { duration_ = value; sourceOut_ = sourceIn_ + value; }
    void setTimelineDuration(ccos::core::Time value) noexcept { if (value > ccos::core::Time{}) duration_ = value; }
    void setSourceRange(ccos::core::Time in, ccos::core::Time out) noexcept;
    void regenerateId() { id_ = ccos::core::Uuid{}; }
    void addEffect(QString id) { if (!id.isEmpty() && !effects_.contains(id)) effects_.append(std::move(id)); }
    void removeEffect(const QString& id) { effects_.removeAll(id); }
private:
    ccos::core::Uuid id_;
    ccos::core::Uuid assetId_;
    ccos::core::Time start_;
    ccos::core::Time duration_;
    ccos::core::Time sourceIn_;
    ccos::core::Time sourceOut_;
    double speed_ = 1.0;
    double audioGain_ = 1.0;
    bool audioMuted_ = false;
    TransformState transform_;
    QStringList effects_;
    QString transitionInId_ = QStringLiteral("cut");
    qint64 transitionInDurationMs_ = 0;
};
}
