#include "render/TimelineExporter.hpp"
#include "render/TimelineCompositor.hpp"
#include "render/HardwareCapabilities.hpp"
#include "core/ProcessRunner.hpp"

#include <QFileInfo>

namespace ccos::render {
namespace {

QString resolveVideoCodec(const QString& requested, const QString& executable) {
    const QString trimmed = requested.trimmed();
    if (trimmed != QStringLiteral("auto")) return trimmed;
    return HardwareCapabilitiesProbe::detect(executable).preferredH264Encoder(true);
}

} // namespace

bool TimelineExporter::exportContiguousVideo(const ccos::project::Project& project,
                                             const QString& outputPath,
                                             const ExportSettings& settings,
                                             const QString& executable,
                                             QString* error) {
    if (outputPath.isEmpty()) {
        if (error) *error = QStringLiteral("Output path is required");
        return false;
    }
    if (!settings.validate(error)) return false;

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
    args << outputPath;

    ccos::core::ProcessRunner runner;
    ccos::core::ProcessConfig config;
    config.executable = executable;
    config.arguments = args;
    config.timeout = std::chrono::minutes(120);
    config.startupTimeout = std::chrono::seconds(5);
    config.maxOutputSize = 32 * 1024 * 1024;
    config.riskLevel = ccos::core::ProcessConfig::RiskLevel::High;

    const auto result = runner.executeSync(config);
    if (!result.isSuccess()) {
        if (error) *error = result.errorMessage();
        return false;
    }

    if (!QFileInfo::exists(outputPath) || QFileInfo(outputPath).size() <= 0) {
        if (error) *error = QStringLiteral("FFmpeg completed without producing the timeline output");
        return false;
    }

    return true;
}
}
