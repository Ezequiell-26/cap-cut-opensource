#pragma once
#include "media/MediaAsset.hpp"
#include <QString>

namespace ccos::media {
class MediaProbe {
public:
    static bool probe(MediaAsset& asset, const QString& executable = QStringLiteral("ffprobe"), QString* error = nullptr);
};
}
