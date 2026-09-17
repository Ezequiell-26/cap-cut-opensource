#include "render/FfmpegExporter.hpp"
#include "core/ProcessRunner.hpp"
#include "render/HardwareCapabilities.hpp"

#include <QFileInfo>

namespace ccos::render {
namespace {

QString resolveVideoCodec(const QString& requested, const QString& executable) {
    const QString trimmed = requested.trimmed();
    if (trimmed != QStringLiteral("auto")) return trimmed;
    return HardwareCapabilitiesProbe::detect(executable).preferredH264Encoder(true);
}

} // namespace

bool FfmpegExporter::exportAsset(const ccos::media::MediaAsset& asset, const QString& outputPath,
                                 const ExportSettings& settings, const QString& executable, QString* error) {
    if (asset.path().isEmpty() || outputPath.isEmpty()) {
        if (error) *error = QStringLiteral("Input and output paths are required");
        return false;
    }

    if (!settings.validate(error)) return false;

    const QString videoCodec = resolveVideoCodec(settings.videoCodec, executable);
    if (videoCodec.isEmpty()) {
        if (error) *error = QStringLiteral("Unable to resolve the requested video encoder");
        return false;
    }

    QStringList args{
        QStringLiteral("-y"), QStringLiteral("-i"), asset.path(),
        QStringLiteral("-vf"), QStringLiteral("scale=%1:%2:force_original_aspect_ratio=decrease,pad=%1:%2:(ow-iw)/2:(oh-ih)/2")
            .arg(settings.width).arg(settings.height),
        QStringLiteral("-r"), QString::number(settings.fps, 'f', 3),
        QStringLiteral("-c:v"), videoCodec,
        QStringLiteral("-b:v"), QStringLiteral("%1k").arg(settings.videoBitrateKbps),
        QStringLiteral("-c:a"), settings.audioCodec,
        QStringLiteral("-b:a"), QStringLiteral("%1k").arg(settings.audioBitrateKbps),
        outputPath
    };

    ccos::core::ProcessRunner runner;
    ccos::core::ProcessConfig config;
    config.executable = executable;
    config.arguments = args;
    config.timeout = std::chrono::minutes(60);
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
        if (error) *error = QStringLiteral("FFmpeg completed without producing the output file");
        return false;
    }

    return true;
}
}
