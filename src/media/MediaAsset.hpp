#pragma once
#include "core/Uuid.hpp"
#include <QString>
#include <cstdint>
#include <utility>

namespace ccos::media {
struct MediaMetadata {
    std::int64_t durationMs = 0;
    std::int32_t width = 0;
    std::int32_t height = 0;
    double fps = 0.0;
    QString videoCodec;
    QString audioCodec;
    std::int32_t audioChannels = 0;
    std::int32_t sampleRate = 0;
};

class MediaAsset {
public:
    MediaAsset();
    explicit MediaAsset(QString path);
    MediaAsset(ccos::core::Uuid id, QString path);
    [[nodiscard]] const ccos::core::Uuid& id() const noexcept { return id_; }
    [[nodiscard]] const QString& path() const noexcept { return path_; }
    void setPath(QString path) { path_ = std::move(path); refreshName(); }
    [[nodiscard]] const QString& name() const noexcept { return name_; }
    [[nodiscard]] const MediaMetadata& metadata() const noexcept { return metadata_; }
    MediaMetadata& metadata() noexcept { return metadata_; }
    void refreshName();
private:
    ccos::core::Uuid id_;
    QString path_;
    QString name_;
    MediaMetadata metadata_;
};
}
