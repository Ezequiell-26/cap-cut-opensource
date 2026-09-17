#pragma once
#include "timeline/Clip.hpp"
#include <QString>
#include <vector>

namespace ccos::timeline {
enum class TrackType { Video, Audio };
class Track {
public:
    explicit Track(TrackType type = TrackType::Video);
    [[nodiscard]] TrackType type() const noexcept { return type_; }
    [[nodiscard]] const QString& name() const noexcept { return name_; }
    void setName(QString name) { name_ = std::move(name); }
    void addClip(const Clip& clip) { clips_.push_back(clip); }
    [[nodiscard]] const std::vector<Clip>& clips() const noexcept { return clips_; }
    std::vector<Clip>& clips() noexcept { return clips_; }
private:
    TrackType type_;
    QString name_;
    std::vector<Clip> clips_;
};
}
