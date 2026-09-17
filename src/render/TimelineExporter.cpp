#include "render/TimelineExporter.hpp"
#include "render/TimelineCompositor.hpp"
#include <QProcess>

namespace ccos::render {

bool TimelineExporter::exportContiguousVideo(const ccos::project::Project& project,
                                             const QString& outputPath,
                                             const ExportSettings& settings,
                                             const QString& executable,
                                             QString* error) {
    if (outputPath.isEmpty()) {
        if (error) *error = QStringLiteral("Output path is required");
        return false;
    }

    QStringList inputs;
    QString filter;
    QString videoMap;
    QString audioMap;
    if (!TimelineCompositor::build(project, settings, inputs, filter, videoMap, audioMap, error)) return false;
    if (inputs.isEmpty()) {
        if (error) *error = QStringLiteral("The timeline contains no media inputs");
        return false;
    }

    QStringList args{QStringLiteral("-hide_banner"), QStringLiteral("-y")};
    for (const auto& input : inputs) args << QStringLiteral("-i") << input;
    args << QStringLiteral("-filter_complex") << filter
         << QStringLiteral("-map") << videoMap
         << QStringLiteral("-map") << audioMap
         << QStringLiteral("-c:v") << settings.videoCodec
         << QStringLiteral("-b:v") << QStringLiteral("%1k").arg(settings.videoBitrateKbps)
         << QStringLiteral("-pix_fmt") << QStringLiteral("yuv420p")
         << QStringLiteral("-r") << QString::number(settings.fps, 'f', 3)
         << QStringLiteral("-c:a") << settings.audioCodec
         << QStringLiteral("-b:a") << QStringLiteral("%1k").arg(settings.audioBitrateKbps)
         << QStringLiteral("-shortest");
    if (settings.container == QStringLiteral("mp4")) args << QStringLiteral("-movflags") << QStringLiteral("+faststart");
    args << outputPath;

    QProcess process;
    process.start(executable, args);
    if (!process.waitForStarted(3000)) {
        if (error) *error = QStringLiteral("Unable to start ffmpeg: %1").arg(process.errorString());
        return false;
    }
    if (!process.waitForFinished(-1) || process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        if (error) *error = QString::fromLocal8Bit(process.readAllStandardError());
        if (error && error->isEmpty()) *error = QStringLiteral("Timeline export failed");
        return false;
    }
    return true;
}
}
