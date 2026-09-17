#pragma once
#include "media/MediaAsset.hpp"
#include <QStringList>
#include <vector>

namespace ccos::media {
class MediaImporter {
public:
    static std::vector<MediaAsset> importFiles(const QStringList& paths);
};
}
