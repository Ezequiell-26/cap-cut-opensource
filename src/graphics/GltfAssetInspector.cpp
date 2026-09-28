#include "graphics/GltfAssetInspector.hpp"

#include <QFileInfo>

#ifdef CCOS_HAS_MIT_MEDIA_3D
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_IMPLEMENTATION
#include <tiny_gltf.h>
#endif

namespace ccos::graphics {

bool GltfAssetInspector::supported() noexcept {
#ifdef CCOS_HAS_MIT_MEDIA_3D
    return true;
#else
    return false;
#endif
}

GltfAssetInfo GltfAssetInspector::inspect(const QString& path) {
    GltfAssetInfo result;
    result.path = path;

    const QFileInfo file(path);
    if (!file.exists() || !file.isFile()) {
        result.error = QStringLiteral("GLTF/GLB file does not exist");
        return result;
    }

    const QString suffix = file.suffix().trimmed().toLower();
    if (suffix != QStringLiteral("gltf") && suffix != QStringLiteral("glb")) {
        result.error = QStringLiteral("Expected a .gltf or .glb file");
        return result;
    }

    constexpr qint64 kMaxFileBytes = 512LL * 1024LL * 1024LL;
    if (file.size() <= 0 || file.size() > kMaxFileBytes) {
        result.error = QStringLiteral("GLTF/GLB file size is outside the supported range");
        return result;
    }

    result.format = suffix.toUpper();

#ifndef CCOS_HAS_MIT_MEDIA_3D
    result.error = QStringLiteral("TinyGLTF support is disabled; configure with -DCCOS_ENABLE_MIT_MEDIA_3D=ON");
    return result;
#else
    tinygltf::TinyGLTF loader;
    tinygltf::Model model;
    std::string error;
    std::string warning;
    bool loaded = false;

    if (suffix == QStringLiteral("glb")) {
        loaded = loader.LoadBinaryFromFile(&model, &error, &warning, path.toStdString());
    } else {
        loaded = loader.LoadASCIIFromFile(&model, &error, &warning, path.toStdString());
    }

    if (!warning.empty() && result.error.isEmpty()) {
        result.error = QString::fromStdString(warning).trimmed();
    }
    if (!loaded) {
        const QString detail = QString::fromStdString(error).trimmed();
        result.error = detail.isEmpty() ? QStringLiteral("TinyGLTF could not parse the asset") : detail;
        return result;
    }

    result.valid = true;
    result.sceneCount = static_cast<int>(model.scenes.size());
    result.nodeCount = static_cast<int>(model.nodes.size());
    result.meshCount = static_cast<int>(model.meshes.size());
    result.materialCount = static_cast<int>(model.materials.size());
    result.imageCount = static_cast<int>(model.images.size());
    result.animationCount = static_cast<int>(model.animations.size());
    result.skinCount = static_cast<int>(model.skins.size());
    return result;
#endif
}

} // namespace ccos::graphics
