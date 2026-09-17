#include "render/HardwareCapabilities.hpp"
#include <QProcess>

namespace ccos::render {
HardwareCapabilities HardwareCapabilitiesProbe::detect(const QString& executable) {
    HardwareCapabilities result;
    QProcess p;
    p.start(executable, {QStringLiteral("-hide_banner"), QStringLiteral("-encoders")} );
    if (!p.waitForStarted(2000) || !p.waitForFinished(5000)) return result;
    const QString text = QString::fromLocal8Bit(p.readAllStandardOutput());
    const QStringList lines = text.split('\n', Qt::SkipEmptyParts);
    for (const QString& line : lines) {
        const QString trimmed = line.trimmed();
        if (trimmed.size() > 2 && !trimmed.startsWith(QChar('—'))) {
            const auto parts = trimmed.split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts);
            if (parts.size() >= 2) result.encoders.append(parts.at(1));
        }
    }
    for (const QString& e : result.encoders) {
        result.hasNvidia |= e.startsWith(QStringLiteral("h264_nvenc")) || e.startsWith(QStringLiteral("hevc_nvenc"));
        result.hasIntel |= e.startsWith(QStringLiteral("h264_qsv")) || e.startsWith(QStringLiteral("hevc_qsv"));
        result.hasAmd |= e.startsWith(QStringLiteral("h264_amf")) || e.startsWith(QStringLiteral("hevc_amf"));
    }
    result.encoders.removeDuplicates();
    return result;
}
}
