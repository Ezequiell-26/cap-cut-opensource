#pragma once
#include "media/MediaAsset.hpp"
#include "render/ExportSettings.hpp"
#include <QString>

namespace ccos::render {
class FfmpegExporter {
public:
    static bool exportAsset(const ccos::media::MediaAsset& asset, const QString& outputPath,
                            const ExportSettings& settings = {}, const QString& executable = QStringLiteral("ffmpeg"),
                            QString* error = nullptr);
};
}
