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

bool samePath(const QString& left, const QString& right) {
    const QString a = QDir::cleanPath(QFileInfo(left).absoluteFilePath());
    const QString b = QDir::cleanPath(QFileInfo(right).absoluteFilePath());
#ifdef Q_OS_WIN
    return QString::compare(a, b, Qt::CaseInsensitive) == 0;
#else
    return a == b;
#endif
}
}

QString ProxyManager::proxyPath(const MediaCache& cache, const MediaAsset& asset, ProxyPreset preset) {
    return cache.pathFor(asset.path(), presetName(preset), QStringLiteral("mp4"));
}

bool ProxyManager::createProxy(const MediaAsset& asset, const QString& outputPath, ProxyPreset preset,
                               const QString& executable, QString* error) {
    if (asset.path().isEmpty() || outputPath.trimmed().isEmpty()) {
        if (error) *error = QStringLiteral("Proxy input and output paths are required");
        return false;
    }
    if (!ccos::core::ProcessRunner::validateExecutable(executable)) {
        if (error) *error = QStringLiteral("FFmpeg executable not found or not executable: %1").arg(executable);
        return false;
    }
    if (samePath(asset.path(), outputPath)) {
        if (error) *error = QStringLiteral("Proxy output path must not overwrite the source media");
        return false;
    }

    const QFileInfo outputInfo(outputPath);
    if (!QDir().mkpath(outputInfo.absolutePath())) {
        if (error) *error = QStringLiteral("Unable to create proxy directory: %1").arg(outputInfo.absolutePath());
        return false;
    }

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
    config.sanitizeEnvironment = true;

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
