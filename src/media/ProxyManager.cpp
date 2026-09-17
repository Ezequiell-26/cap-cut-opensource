#include "media/ProxyManager.hpp"
#include "media/MediaCache.hpp"
#include "core/ProcessRunner.hpp"

#include <QDir>
#include <QFileInfo>

namespace ccos::media {
namespace {
int heightFor(ProxyPreset p) {
    switch (p) {
    case ProxyPreset::Quarter: return 540;
    case ProxyPreset::Half: return 1080;
    case ProxyPreset::P720: return 720;
    case ProxyPreset::P1080: return 1080;
    }
    return 540;
}

QString presetName(ProxyPreset p) {
    switch (p) {
    case ProxyPreset::Quarter: return QStringLiteral("proxy_q");
    case ProxyPreset::Half: return QStringLiteral("proxy_h");
    case ProxyPreset::P720: return QStringLiteral("proxy_720");
    case ProxyPreset::P1080: return QStringLiteral("proxy_1080");
    }
    return QStringLiteral("proxy_q");
}
}

QString ProxyManager::proxyPath(const MediaCache& cache, const MediaAsset& asset, ProxyPreset preset) {
    return cache.pathFor(asset.path(), presetName(preset), QStringLiteral("mp4"));
}

bool ProxyManager::createProxy(const MediaAsset& asset, const QString& outputPath, ProxyPreset preset,
                               const QString& executable, QString* error) {
    if (asset.path().isEmpty() || outputPath.isEmpty()) {
        if (error) *error = QStringLiteral("Proxy input and output paths are required");
        return false;
    }

    QDir().mkpath(QFileInfo(outputPath).absolutePath());
    const QString vf = QStringLiteral("scale=-2:%1:flags=lanczos").arg(heightFor(preset));

    ccos::core::ProcessRunner runner;
    ccos::core::ProcessConfig config;
    config.executable = executable;
    config.arguments = {
        QStringLiteral("-y"), QStringLiteral("-i"), asset.path(),
        QStringLiteral("-vf"), vf,
        QStringLiteral("-c:v"), QStringLiteral("libx264"),
        QStringLiteral("-preset"), QStringLiteral("veryfast"),
        QStringLiteral("-crf"), QStringLiteral("23"),
        QStringLiteral("-c:a"), QStringLiteral("aac"),
        QStringLiteral("-b:a"), QStringLiteral("128k"),
        outputPath
    };
    config.timeout = std::chrono::minutes(30);
    config.startupTimeout = std::chrono::seconds(5);
    config.maxOutputSize = 16 * 1024 * 1024;
    config.riskLevel = ccos::core::ProcessConfig::RiskLevel::High;

    const auto result = runner.executeSync(config);
    if (!result.isSuccess()) {
        if (error) *error = result.errorMessage();
        return false;
    }

    if (!QFileInfo::exists(outputPath) || QFileInfo(outputPath).size() <= 0) {
        if (error) *error = QStringLiteral("FFmpeg completed without producing a proxy file");
        return false;
    }
    return true;
}

bool ProxyManager::ensureProxy(const MediaCache& cache, const MediaAsset& asset, ProxyPreset preset,
                               const QString& executable, QString* error) {
    const QString output = proxyPath(cache, asset, preset);
    if (QFileInfo::exists(output) && QFileInfo(output).size() > 0) return true;
    return createProxy(asset, output, preset, executable, error);
}
}
