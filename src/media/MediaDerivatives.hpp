#pragma once
#include "media/MediaAsset.hpp"
#include "media/MediaCache.hpp"
#include <QString>

namespace ccos::media {
class MediaDerivatives {
public:
    static QString thumbnailPath(const MediaCache& cache, const MediaAsset& asset, qint64 timeMs = 0);
    static QString waveformPath(const MediaCache& cache, const MediaAsset& asset);
    static bool createThumbnail(const MediaAsset& asset, const QString& outputPath, qint64 timeMs = 0,
                                const QString& executable = QStringLiteral("ffmpeg"), QString* error = nullptr);
    static bool createWaveform(const MediaAsset& asset, const QString& outputPath,
                               const QString& executable = QStringLiteral("ffmpeg"), QString* error = nullptr);
};
}
