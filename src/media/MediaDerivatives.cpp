#include "media/MediaDerivatives.hpp"
#include "core/ProcessRunner.hpp"

#include <QDir>
#include <QFileInfo>

namespace ccos::media {
namespace {
bool run(const QStringList& args, const QString& executable, std::chrono::milliseconds timeout, QString* error) {
    ccos::core::ProcessRunner runner;
    ccos::core::ProcessConfig config;
    config.executable = executable;
    config.arguments = args;
    config.timeout = timeout;
    config.startupTimeout = std::chrono::seconds(5);
    config.maxOutputSize = 4 * 1024 * 1024;
    config.riskLevel = ccos::core::ProcessConfig::RiskLevel::Medium;

    const auto result = runner.executeSync(config);
    if (!result.isSuccess()) {
        if (error) *error = result.errorMessage();
        return false;
    }
    return true;
}
}

QString MediaDerivatives::thumbnailPath(const MediaCache& cache, const MediaAsset& asset, qint64 timeMs) {
    return cache.pathFor(asset.path(), QStringLiteral("thumb_%1").arg(timeMs), QStringLiteral("jpg"));
}

QString MediaDerivatives::waveformPath(const MediaCache& cache, const MediaAsset& asset) {
    return cache.pathFor(asset.path(), QStringLiteral("waveform"), QStringLiteral("png"));
}

bool MediaDerivatives::createThumbnail(const MediaAsset& asset, const QString& outputPath, qint64 timeMs,
                                       const QString& executable, QString* error) {
    if (asset.path().isEmpty() || outputPath.isEmpty()) {
        if (error) *error = QStringLiteral("Thumbnail input/output required");
        return false;
    }
    QDir().mkpath(QFileInfo(outputPath).absolutePath());
    return run({QStringLiteral("-y"), QStringLiteral("-ss"), QString::number(timeMs / 1000.0, 'f', 3),
                QStringLiteral("-i"), asset.path(), QStringLiteral("-frames:v"), QStringLiteral("1"),
                QStringLiteral("-q:v"), QStringLiteral("3"), outputPath},
               executable, std::chrono::seconds(30), error);
}

bool MediaDerivatives::createWaveform(const MediaAsset& asset, const QString& outputPath,
                                      const QString& executable, QString* error) {
    if (asset.path().isEmpty() || outputPath.isEmpty()) {
        if (error) *error = QStringLiteral("Waveform input/output required");
        return false;
    }
    QDir().mkpath(QFileInfo(outputPath).absolutePath());
    return run({QStringLiteral("-y"), QStringLiteral("-i"), asset.path(), QStringLiteral("-filter_complex"),
                QStringLiteral("aformat=channel_layouts=mono,showwavespic=s=1600x260:colors=white"),
                QStringLiteral("-frames:v"), QStringLiteral("1"), outputPath},
               executable, std::chrono::minutes(2), error);
}
}
