#include "render/FfmpegExporter.hpp"
#include <QProcess>

namespace ccos::render {

bool FfmpegExporter::exportAsset(const ccos::media::MediaAsset& asset, const QString& outputPath,
                                 const ExportSettings& settings, const QString& executable, QString* error) {
    if (asset.path().isEmpty() || outputPath.isEmpty()) {
        if (error) *error = QStringLiteral("Input and output paths are required");
        return false;
    }

    QProcess process;
    QStringList args{
        QStringLiteral("-y"), QStringLiteral("-i"), asset.path(),
        QStringLiteral("-vf"), QStringLiteral("scale=%1:%2:force_original_aspect_ratio=decrease,pad=%1:%2:(ow-iw)/2:(oh-ih)/2")
            .arg(settings.width).arg(settings.height),
        QStringLiteral("-r"), QString::number(settings.fps, 'f', 3),
        QStringLiteral("-c:v"), settings.videoCodec,
        QStringLiteral("-b:v"), QStringLiteral("%1k").arg(settings.videoBitrateKbps),
        QStringLiteral("-c:a"), settings.audioCodec,
        QStringLiteral("-b:a"), QStringLiteral("%1k").arg(settings.audioBitrateKbps),
        outputPath
    };

    process.start(executable, args);
    if (!process.waitForStarted(3000)) {
        if (error) *error = QStringLiteral("Unable to start ffmpeg: %1").arg(process.errorString());
        return false;
    }
    if (!process.waitForFinished(-1) || process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        if (error) *error = QString::fromLocal8Bit(process.readAllStandardError());
        if (error && error->isEmpty()) *error = QStringLiteral("ffmpeg export failed");
        return false;
    }
    return true;
}
}
