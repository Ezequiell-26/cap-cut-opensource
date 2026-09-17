#pragma once
#include "media/MediaAsset.hpp"
#include <QString>

namespace ccos::media {
class MediaCache;

enum class ProxyPreset { Quarter, Half, P720, P1080 };

class ProxyManager {
public:
    static QString proxyPath(const MediaCache& cache, const MediaAsset& asset, ProxyPreset preset);
    static bool createProxy(const MediaAsset& asset, const QString& outputPath,
                            ProxyPreset preset = ProxyPreset::Quarter,
                            const QString& executable = QStringLiteral("ffmpeg"),
                            QString* error = nullptr);
    static bool ensureProxy(const MediaCache& cache, const MediaAsset& asset,
                            ProxyPreset preset = ProxyPreset::Quarter,
                            const QString& executable = QStringLiteral("ffmpeg"),
                            QString* error = nullptr);
};
}
