#include "media/ProxyManager.hpp"
#include "media/MediaCache.hpp"
#include <QFileInfo>
#include <QProcess>
#include <QDir>

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
    QProcess p;
    const QString vf = QStringLiteral("scale=-2:%1:flags=lanczos").arg(heightFor(preset));
    p.start(executable, {QStringLiteral("-y"), QStringLiteral("-i"), asset.path(),
                         QStringLiteral("-vf"), vf, QStringLiteral("-c:v"), QStringLiteral("libx264"),
                         QStringLiteral("-preset"), QStringLiteral("veryfast"), QStringLiteral("-crf"), QStringLiteral("23"),
                         QStringLiteral("-c:a"), QStringLiteral("aac"), QStringLiteral("-b:a"), QStringLiteral("128k"),
                         outputPath});
    if (!p.waitForStarted(3000)) {
        if (error) *error = QStringLiteral("Unable to start ffmpeg for proxy: %1").arg(p.errorString());
        return false;
    }
    if (!p.waitForFinished(-1) || p.exitStatus() != QProcess::NormalExit || p.exitCode() != 0) {
        if (error) *error = QString::fromLocal8Bit(p.readAllStandardError());
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
