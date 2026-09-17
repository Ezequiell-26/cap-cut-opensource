#include "render/ExportPresets.hpp"

namespace ccos::render {
namespace {

ExportSettings makeH264(int width, int height, double fps, int bitrateKbps) {
    ExportSettings settings;
    settings.width = width;
    settings.height = height;
    settings.fps = fps;
    settings.container = QStringLiteral("mp4");
    settings.videoCodec = QStringLiteral("libx264");
    settings.audioCodec = QStringLiteral("aac");
    settings.videoBitrateKbps = bitrateKbps;
    settings.audioBitrateKbps = 192;
    return settings;
}

} // namespace

QList<ExportPreset> ExportPresetCatalog::all() {
    return {
        {ExportPresetId::H264_1080p30, QStringLiteral("1080p 30 fps"),
         QStringLiteral("Full HD H.264 export at 30 fps"), settingsFor(ExportPresetId::H264_1080p30)},
        {ExportPresetId::H264_1080p60, QStringLiteral("1080p 60 fps"),
         QStringLiteral("Full HD H.264 export at 60 fps"), settingsFor(ExportPresetId::H264_1080p60)},
        {ExportPresetId::H264_4K30, QStringLiteral("4K 30 fps"),
         QStringLiteral("UHD H.264 export at 30 fps"), settingsFor(ExportPresetId::H264_4K30)},
        {ExportPresetId::H264_Vertical1080p30, QStringLiteral("Vertical 1080p 30 fps"),
         QStringLiteral("Portrait H.264 export at 1080 × 1920"), settingsFor(ExportPresetId::H264_Vertical1080p30)},
        {ExportPresetId::WebM_1080p30, QStringLiteral("WebM 1080p 30 fps"),
         QStringLiteral("WebM export using VP9 and Opus"), settingsFor(ExportPresetId::WebM_1080p30)}
    };
}

ExportSettings ExportPresetCatalog::settingsFor(ExportPresetId id) {
    switch (id) {
    case ExportPresetId::H264_1080p30:
        return makeH264(1920, 1080, 30.0, 8000);
    case ExportPresetId::H264_1080p60:
        return makeH264(1920, 1080, 60.0, 12000);
    case ExportPresetId::H264_4K30:
        return makeH264(3840, 2160, 30.0, 24000);
    case ExportPresetId::H264_Vertical1080p30:
        return makeH264(1080, 1920, 30.0, 8000);
    case ExportPresetId::WebM_1080p30: {
        ExportSettings settings;
        settings.width = 1920;
        settings.height = 1080;
        settings.fps = 30.0;
        settings.container = QStringLiteral("webm");
        settings.videoCodec = QStringLiteral("libvpx-vp9");
        settings.audioCodec = QStringLiteral("libopus");
        settings.videoBitrateKbps = 8000;
        settings.audioBitrateKbps = 160;
        return settings;
    }
    }
    return {};
}

QString ExportPresetCatalog::nameFor(ExportPresetId id) {
    switch (id) {
    case ExportPresetId::H264_1080p30: return QStringLiteral("1080p 30 fps");
    case ExportPresetId::H264_1080p60: return QStringLiteral("1080p 60 fps");
    case ExportPresetId::H264_4K30: return QStringLiteral("4K 30 fps");
    case ExportPresetId::H264_Vertical1080p30: return QStringLiteral("Vertical 1080p 30 fps");
    case ExportPresetId::WebM_1080p30: return QStringLiteral("WebM 1080p 30 fps");
    }
    return {};
}

QString ExportPresetCatalog::idString(ExportPresetId id) {
    switch (id) {
    case ExportPresetId::H264_1080p30: return QStringLiteral("h264-1080p30");
    case ExportPresetId::H264_1080p60: return QStringLiteral("h264-1080p60");
    case ExportPresetId::H264_4K30: return QStringLiteral("h264-4k30");
    case ExportPresetId::H264_Vertical1080p30: return QStringLiteral("h264-vertical1080p30");
    case ExportPresetId::WebM_1080p30: return QStringLiteral("webm-1080p30");
    }
    return {};
}

bool ExportPresetCatalog::fromIdString(const QString& id, ExportPresetId* result) {
    if (!result) return false;
    const QString normalized = id.trimmed().toLower();
    if (normalized == QStringLiteral("h264-1080p30")) { *result = ExportPresetId::H264_1080p30; return true; }
    if (normalized == QStringLiteral("h264-1080p60")) { *result = ExportPresetId::H264_1080p60; return true; }
    if (normalized == QStringLiteral("h264-4k30")) { *result = ExportPresetId::H264_4K30; return true; }
    if (normalized == QStringLiteral("h264-vertical1080p30")) { *result = ExportPresetId::H264_Vertical1080p30; return true; }
    if (normalized == QStringLiteral("webm-1080p30")) { *result = ExportPresetId::WebM_1080p30; return true; }
    return false;
}

} // namespace ccos::render
