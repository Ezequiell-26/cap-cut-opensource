#pragma once
#include "media/MediaAsset.hpp"
#include <QString>
#include <QStringList>
#include <cstddef>
#include <vector>

namespace ccos::media {
class MediaImporter {
public:
    static std::vector<MediaAsset> importFiles(const QStringList& paths);
    static std::vector<MediaAsset> importFiles(const QStringList& paths, std::size_t maxFiles, QString* error);
};
}
