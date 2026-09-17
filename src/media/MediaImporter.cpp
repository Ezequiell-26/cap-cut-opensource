#include "media/MediaImporter.hpp"
#include "media/MediaProbe.hpp"

std::vector<ccos::media::MediaAsset> ccos::media::MediaImporter::importFiles(const QStringList& paths) {
    std::vector<MediaAsset> assets;
    assets.reserve(static_cast<std::size_t>(paths.size()));
    for (const auto& path : paths) {
        if (path.isEmpty()) continue;
        MediaAsset asset(path);
        QString ignoredError;
        MediaProbe::probe(asset, QStringLiteral("ffprobe"), &ignoredError);
        assets.push_back(std::move(asset));
    }
    return assets;
}
