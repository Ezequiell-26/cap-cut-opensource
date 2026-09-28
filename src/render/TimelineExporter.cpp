#include "render/TimelineExporter.hpp"
#include "render/TimelineCompositor.hpp"
#include "render/HardwareCapabilities.hpp"
#include "core/ProcessRunner.hpp"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QUuid>

namespace ccos::render {
namespace {

QString resolveVideoCodec(const QString& requested, const QString& executable) {
    const QString trimmed = requested.trimmed();
    if (trimmed != QStringLiteral("auto")) return trimmed;
    return HardwareCapabilitiesProbe::detect(executable).preferredH264Encoder(true);
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

QString uniqueSiblingPath(const QString& outputPath, const QString& marker) {
    const QFileInfo info(outputPath);
    const QString baseName = info.completeBaseName().isEmpty()
        ? QStringLiteral("output")
        : info.completeBaseName();
    const QString suffix = info.suffix();
    QString path;
    do {
        path = QDir(info.absolutePath()).filePath(
            QStringLiteral(".%1.ccos-%2%3")
                .arg(baseName, marker, QUuid::createUuid().toString(QUuid::WithoutBraces)));
        if (!suffix.isEmpty()) path += QStringLiteral(".") + suffix;
    } while (QFileInfo::exists(path));
    return path;
}

} // namespace

bool TimelineExporter::exportContiguousVideo(const ccos::project::Project& project,
                                             const QString& outputPath,
                                             const ExportSettings& settings,
                                             const QString& executable,
                                             QString* error) {
    if (outputPath.trimmed().isEmpty()) {
        if (error) *error = QStringLiteral("Output path is required");
        return false;
    }
    if (!settings.validate(error)) return false;
    if (!ccos::core::ProcessRunner::validateExecutable(executable)) {
        if (error) *error = QStringLiteral("FFmpeg executable not found or not executable: %1").arg(executable);
        return false;
    }

    const QString videoCodec = resolveVideoCodec(settings.videoCodec, executable);
    if (videoCodec.isEmpty()) {
        if (error) *error = QStringLiteral("Unable to resolve the requested video encoder");
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
    for (const auto& input : inputs) {
        if (samePath(input, outputPath)) {
            if (error) *error = QStringLiteral("Output path must not overwrite a timeline input");
            return false;
        }
    }

    const QFileInfo outputInfo(outputPath);
    if (!QDir().mkpath(outputInfo.absolutePath())) {
        if (error) *error = QStringLiteral("Unable to create output directory: %1").arg(outputInfo.absolutePath());
        return false;
    }

    const QString temporaryOutputPath = uniqueSiblingPath(outputPath, QStringLiteral("tmp"));

    QStringList args{QStringLiteral("-hide_banner"), QStringLiteral("-y")};
    for (const auto& input : inputs) args << QStringLiteral("-i") << input;
    args << QStringLiteral("-filter_complex") << filter
         << QStringLiteral("-map") << videoMap
         << QStringLiteral("-map") << audioMap
         << QStringLiteral("-c:v") << videoCodec
         << QStringLiteral("-b:v") << QStringLiteral("%1k").arg(settings.videoBitrateKbps)
         << QStringLiteral("-pix_fmt") << QStringLiteral("yuv420p")
         << QStringLiteral("-r") << QString::number(settings.fps, 'f', 3)
         << QStringLiteral("-c:a") << settings.audioCodec
         << QStringLiteral("-b:a") << QStringLiteral("%1k").arg(settings.audioBitrateKbps)
         << QStringLiteral("-shortest");
    if (settings.container == QStringLiteral("mp4")) args << QStringLiteral("-movflags") << QStringLiteral("+faststart");
    args << temporaryOutputPath;

    ccos::core::ProcessRunner runner;
    ccos::core::ProcessConfig config;
    config.executable = executable;
    config.arguments = args;
    config.timeout = std::chrono::minutes(120);
    config.startupTimeout = std::chrono::seconds(5);
    config.maxOutputSize = 32 * 1024 * 1024;
    config.riskLevel = ccos::core::ProcessConfig::RiskLevel::High;
    config.sanitizeEnvironment = true;

    const auto result = runner.executeSync(config);
    if (!result.isSuccess()) {
        QFile::remove(temporaryOutputPath);
        if (error) *error = result.errorMessage();
        return false;
    }

    if (!QFileInfo::exists(temporaryOutputPath) || QFileInfo(temporaryOutputPath).size() <= 0) {
        QFile::remove(temporaryOutputPath);
        if (error) *error = QStringLiteral("FFmpeg completed without producing the timeline output");
        return false;
    }

    QString backupPath;
    if (QFileInfo::exists(outputPath)) {
        backupPath = uniqueSiblingPath(outputPath, QStringLiteral("backup"));
        if (!QFile::rename(outputPath, backupPath)) {
            QFile::remove(temporaryOutputPath);
            if (error) *error = QStringLiteral("Unable to protect existing output before export: %1").arg(outputPath);
            return false;
        }
    }

    if (!QFile::rename(temporaryOutputPath, outputPath)) {
        if (!backupPath.isEmpty()) QFile::rename(backupPath, outputPath);
        QFile::remove(temporaryOutputPath);
        if (error) *error = QStringLiteral("Unable to finalize exported timeline: %1").arg(outputPath);
        return false;
    }

    if (!backupPath.isEmpty()) QFile::remove(backupPath);
    return true;
}
}
