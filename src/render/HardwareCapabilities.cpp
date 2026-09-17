#include "render/HardwareCapabilities.hpp"
#include "core/ProcessRunner.hpp"

#include <QRegularExpression>

#include <utility>

namespace ccos::render {

QString HardwareCapabilities::preferredH264Encoder(bool hardwareAcceleration) const {
    if (!hardwareAcceleration) return QStringLiteral("libx264");
    // These encoders can consume the software frames produced by the current
    // CCOS FFmpeg pipeline without requiring a device/upload filter setup.
    if (supports(QStringLiteral("h264_nvenc"))) return QStringLiteral("h264_nvenc");
    if (supports(QStringLiteral("h264_qsv"))) return QStringLiteral("h264_qsv");
    if (supports(QStringLiteral("h264_amf"))) return QStringLiteral("h264_amf");
    if (supports(QStringLiteral("h264_videotoolbox"))) return QStringLiteral("h264_videotoolbox");
    return QStringLiteral("libx264");
}

QString HardwareCapabilities::preferredHevcEncoder(bool hardwareAcceleration) const {
    if (!hardwareAcceleration) return QStringLiteral("libx265");
    if (supports(QStringLiteral("hevc_nvenc"))) return QStringLiteral("hevc_nvenc");
    if (supports(QStringLiteral("hevc_qsv"))) return QStringLiteral("hevc_qsv");
    if (supports(QStringLiteral("hevc_amf"))) return QStringLiteral("hevc_amf");
    if (supports(QStringLiteral("hevc_videotoolbox"))) return QStringLiteral("hevc_videotoolbox");
    return QStringLiteral("libx265");
}

HardwareCapabilities HardwareCapabilitiesProbe::detect(const QString& executable) {
    HardwareCapabilities result;

    ccos::core::ProcessRunner runner;
    ccos::core::ProcessConfig config;
    config.executable = executable;
    config.arguments = {QStringLiteral("-hide_banner"), QStringLiteral("-encoders")};
    config.startupTimeout = std::chrono::seconds(3);
    config.timeout = std::chrono::seconds(8);
    config.maxOutputSize = 8 * 1024 * 1024;
    config.riskLevel = ccos::core::ProcessConfig::RiskLevel::Low;

    const auto process = runner.executeSync(config);
    if (!process.isSuccess()) return result;

    const QString text = QString::fromLocal8Bit(process.standardOutput);
    for (const QString& line : text.split('\n', Qt::SkipEmptyParts)) {
        const QString trimmed = line.trimmed();
        const auto parts = trimmed.split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts);
        if (parts.size() >= 2 && !parts.at(0).startsWith(QLatin1Char('['))) {
            result.encoders.append(parts.at(1));
        }
    }

    for (const QString& encoder : std::as_const(result.encoders)) {
        result.hasNvidia |= encoder.startsWith(QStringLiteral("h264_nvenc")) || encoder.startsWith(QStringLiteral("hevc_nvenc"));
        result.hasIntel |= encoder.startsWith(QStringLiteral("h264_qsv")) || encoder.startsWith(QStringLiteral("hevc_qsv"));
        result.hasAmd |= encoder.startsWith(QStringLiteral("h264_amf")) || encoder.startsWith(QStringLiteral("hevc_amf"));
        result.hasVideoToolbox |= encoder.startsWith(QStringLiteral("h264_videotoolbox")) || encoder.startsWith(QStringLiteral("hevc_videotoolbox"));
        result.hasVaapi |= encoder.startsWith(QStringLiteral("h264_vaapi")) || encoder.startsWith(QStringLiteral("hevc_vaapi"));
    }

    result.encoders.removeDuplicates();
    return result;
}

} // namespace ccos::render
