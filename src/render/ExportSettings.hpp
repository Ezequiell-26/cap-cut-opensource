#pragma once
#include <QString>
#include <cstdint>

namespace ccos::render {
struct ExportSettings {
    std::int32_t width = 1920;
    std::int32_t height = 1080;
    double fps = 30.0;
    QString container = QStringLiteral("mp4");
    QString videoCodec = QStringLiteral("h264");
    QString audioCodec = QStringLiteral("aac");
    std::int32_t videoBitrateKbps = 8000;
    std::int32_t audioBitrateKbps = 192;
};
}
