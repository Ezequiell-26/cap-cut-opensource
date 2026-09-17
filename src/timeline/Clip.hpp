#pragma once
#include "core/Time.hpp"
#include "core/Uuid.hpp"
#include "media/MediaAsset.hpp"

namespace ccos::timeline {
class Clip {
public:
    Clip();
    explicit Clip(const ccos::media::MediaAsset& asset);
    [[nodiscard]] const ccos::core::Uuid& id() const noexcept { return id_; }
    [[nodiscard]] const ccos::core::Uuid& assetId() const noexcept { return assetId_; }
    [[nodiscard]] ccos::core::Time start() const noexcept { return start_; }
    [[nodiscard]] ccos::core::Time duration() const noexcept { return duration_; }
    void setStart(ccos::core::Time value) noexcept { start_ = value; }
    void setDuration(ccos::core::Time value) noexcept { duration_ = value; }
private:
    ccos::core::Uuid id_;
    ccos::core::Uuid assetId_;
    ccos::core::Time start_;
    ccos::core::Time duration_;
};
}
