#pragma once

#include "render/ExportSettings.hpp"

#include <QList>
#include <QString>
#include <QStringList>

namespace ccos::render {

enum class ExportPresetId {
    H264_1080p30,
    H264_1080p60,
    H264_4K30,
    H264_Vertical1080p30,
    WebM_1080p30
};

struct ExportPreset {
    ExportPresetId id;
    QString name;
    QString description;
    ExportSettings settings;
};

class ExportPresetCatalog {
public:
    [[nodiscard]] static QList<ExportPreset> all();
    [[nodiscard]] static ExportSettings settingsFor(ExportPresetId id);
    [[nodiscard]] static QString nameFor(ExportPresetId id);
    [[nodiscard]] static QString idString(ExportPresetId id);
    [[nodiscard]] static bool fromIdString(const QString& id, ExportPresetId* result);
};

} // namespace ccos::render
