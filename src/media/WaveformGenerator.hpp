#pragma once

#include "media/MediaAsset.hpp"

#include <QString>
#include <QStringList>

namespace ccos::media {

class MediaCache;

class WaveformGenerator {
public:
    static QString waveformPath(const MediaCache& cache,
                                const MediaAsset& asset,
                                int width = 1200,
                                int height = 160,
                                int samplesPerSecond = 100);

    static QStringList buildArguments(const MediaAsset& asset,
                                      const QString& outputPath,
                                      int width,
                                      int height,
                                      int samplesPerSecond,
                                      QString* error = nullptr);

    static bool generate(const MediaAsset& asset,
                         const QString& outputPath,
                         int width = 1200,
                         int height = 160,
                         int samplesPerSecond = 100,
                         const QString& executable = QStringLiteral("ffmpeg"),
                         QString* error = nullptr);

    static bool ensure(const MediaCache& cache,
                       const MediaAsset& asset,
                       int width = 1200,
                       int height = 160,
                       int samplesPerSecond = 100,
                       const QString& executable = QStringLiteral("ffmpeg"),
                       QString* error = nullptr);
};

} // namespace ccos::media
