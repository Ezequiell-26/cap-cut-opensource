#include "media/MediaImporter.hpp"

std::vector<ccos::media::MediaAsset> ccos::media::MediaImporter::importFiles(const QStringList& paths) {
    std::vector<MediaAsset> assets;
    assets.reserve(static_cast<std::size_t>(paths.size()));
    for (const auto& path : paths) {
        if (!path.isEmpty()) {
            assets.emplace_back(path);
        }
    }
    return assets;
}
