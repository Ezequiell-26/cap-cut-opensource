#pragma once
#include "core/Time.hpp"
#include "core/Uuid.hpp"
#include "media/MediaAsset.hpp"
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
    [[nodiscard]] const TransformState& transform() const noexcept { return transform_; }
    TransformState& transform() noexcept { return transform_; }
    [[nodiscard]] const QStringList& effects() const noexcept { return effects_; }
    QStringList& effects() noexcept { return effects_; }
    void setSpeed(double value) noexcept { speed_ = value > 0.0 ? value : 1.0; }
    void setStart(ccos::core::Time value) noexcept { start_ = value; }
    void setDuration(ccos::core::Time value) noexcept { duration_ = value; sourceOut_ = sourceIn_ + value; }
    void setSourceRange(ccos::core::Time in, ccos::core::Time out) noexcept;
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
    TransformState transform_;
    QStringList effects_;
};
}
