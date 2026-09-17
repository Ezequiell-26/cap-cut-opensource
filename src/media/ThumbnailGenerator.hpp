#pragma once

#include "media/MediaAsset.hpp"

#include <QString>
#include <QStringList>

namespace ccos::media {

class MediaCache;

class ThumbnailGenerator {
public:
    static QString thumbnailPath(const MediaCache& cache,
                                 const MediaAsset& asset,
                                 qint64 positionMs,
                                 int width = 320,
                                 int height = 180);

    static bool generate(const MediaAsset& asset,
                         const QString& outputPath,
                         qint64 positionMs = 0,
                         int width = 320,
                         int height = 180,
                         const QString& executable = QStringLiteral("ffmpeg"),
                         QString* error = nullptr);

    static bool ensure(const MediaCache& cache,
                       const MediaAsset& asset,
                       qint64 positionMs = 0,
                       int width = 320,
                       int height = 180,
                       const QString& executable = QStringLiteral("ffmpeg"),
                       QString* error = nullptr);

    // Exposed for deterministic unit tests and agent-side command inspection.
    static QStringList buildArguments(const MediaAsset& asset,
                                      const QString& outputPath,
                                      qint64 positionMs,
                                      int width,
                                      int height,
                                      QString* error = nullptr);
};

} // namespace ccos::media
