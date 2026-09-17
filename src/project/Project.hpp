#pragma once
#include "media/MediaAsset.hpp"
#include "timeline/Timeline.hpp"
#include "core/Uuid.hpp"
#include <QString>
#include <vector>

namespace ccos::project {
class Project {
public:
    Project();
    explicit Project(QString name);
    [[nodiscard]] const ccos::core::Uuid& id() const noexcept { return id_; }
    [[nodiscard]] const QString& name() const noexcept { return name_; }
    void setName(QString name) { name_ = std::move(name); }
    [[nodiscard]] const std::vector<ccos::media::MediaAsset>& assets() const noexcept { return assets_; }
    std::vector<ccos::media::MediaAsset>& assets() noexcept { return assets_; }
    [[nodiscard]] ccos::timeline::Timeline& timeline() noexcept { return timeline_; }
    [[nodiscard]] const ccos::timeline::Timeline& timeline() const noexcept { return timeline_; }
    void addAsset(ccos::media::MediaAsset asset) { assets_.push_back(std::move(asset)); }
private:
    ccos::core::Uuid id_;
    QString name_;
    std::vector<ccos::media::MediaAsset> assets_;
    ccos::timeline::Timeline timeline_;
};
}
