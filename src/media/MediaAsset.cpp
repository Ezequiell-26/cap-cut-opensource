#include "media/MediaAsset.hpp"
#include <QFileInfo>

namespace ccos::media {
MediaAsset::MediaAsset() = default;
MediaAsset::MediaAsset(QString path) : path_(std::move(path)) { refreshName(); }
MediaAsset::MediaAsset(ccos::core::Uuid id, QString path) : id_(std::move(id)), path_(std::move(path)) { refreshName(); }
void MediaAsset::refreshName() { name_ = QFileInfo(path_).fileName(); }
}
