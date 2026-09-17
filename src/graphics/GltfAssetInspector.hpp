#pragma once

#include <QString>

namespace ccos::graphics {

struct GltfAssetInfo {
    bool valid = false;
    QString path;
    QString format;
    QString error;
    int sceneCount = 0;
    int nodeCount = 0;
    int meshCount = 0;
    int materialCount = 0;
    int imageCount = 0;
    int animationCount = 0;
    int skinCount = 0;
};

class GltfAssetInspector final {
public:
    [[nodiscard]] static bool supported() noexcept;
    [[nodiscard]] static GltfAssetInfo inspect(const QString& path);
};

} // namespace ccos::graphics
