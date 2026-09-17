#pragma once
#include <QString>
#include <QStringList>

namespace ccos::render {

struct HardwareCapabilities {
    QStringList encoders;
    bool hasNvidia = false;
    bool hasIntel = false;
    bool hasAmd = false;
    bool hasVideoToolbox = false;
    bool hasVaapi = false;

    [[nodiscard]] bool supports(const QString& encoder) const { return encoders.contains(encoder); }

    [[nodiscard]] QString preferredH264Encoder(bool hardwareAcceleration = true) const;
    [[nodiscard]] QString preferredHevcEncoder(bool hardwareAcceleration = true) const;
};

class HardwareCapabilitiesProbe {
public:
    static HardwareCapabilities detect(const QString& ffmpegExecutable = QStringLiteral("ffmpeg"));
};

} // namespace ccos::render
