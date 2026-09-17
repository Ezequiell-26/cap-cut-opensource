#pragma once
#include <QStringList>

namespace ccos::render {
struct HardwareCapabilities {
    QStringList encoders;
    bool hasNvidia = false;
    bool hasIntel = false;
    bool hasAmd = false;
    [[nodiscard]] bool supports(const QString& encoder) const { return encoders.contains(encoder); }
};

class HardwareCapabilitiesProbe {
public:
    static HardwareCapabilities detect(const QString& ffmpegExecutable = QStringLiteral("ffmpeg"));
};
}
