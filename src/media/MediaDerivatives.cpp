#include "media/MediaDerivatives.hpp"
#include <QDir>
#include <QFileInfo>
#include <QProcess>

namespace ccos::media {
namespace {
QString run(const QStringList& args, const QString& executable, QString* error) {
    QProcess p;
    p.start(executable, args);
    if (!p.waitForStarted(3000)) { if (error) *error = p.errorString(); return {}; }
    if (!p.waitForFinished(-1) || p.exitStatus() != QProcess::NormalExit || p.exitCode() != 0) {
        if (error) *error = QString::fromLocal8Bit(p.readAllStandardError());
        return {};
    }
    return QStringLiteral("ok");
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
    if (asset.path().isEmpty() || outputPath.isEmpty()) { if (error) *error = QStringLiteral("Thumbnail input/output required"); return false; }
    QDir().mkpath(QFileInfo(outputPath).absolutePath());
    return !run({QStringLiteral("-y"), QStringLiteral("-ss"), QString::number(timeMs / 1000.0, 'f', 3),
                 QStringLiteral("-i"), asset.path(), QStringLiteral("-frames:v"), QStringLiteral("1"),
                 QStringLiteral("-q:v"), QStringLiteral("3"), outputPath}, executable, error).isEmpty();
}

bool MediaDerivatives::createWaveform(const MediaAsset& asset, const QString& outputPath,
                                      const QString& executable, QString* error) {
    if (asset.path().isEmpty() || outputPath.isEmpty()) { if (error) *error = QStringLiteral("Waveform input/output required"); return false; }
    QDir().mkpath(QFileInfo(outputPath).absolutePath());
    return !run({QStringLiteral("-y"), QStringLiteral("-i"), asset.path(), QStringLiteral("-filter_complex"),
                 QStringLiteral("aformat=channel_layouts=mono,showwavespic=s=1600x260:colors=white"),
                 QStringLiteral("-frames:v"), QStringLiteral("1"), outputPath}, executable, error).isEmpty();
}
}
