#pragma once
#include "project/Project.hpp"
#include "render/ExportSettings.hpp"
#include <QString>

namespace ccos::render {
class TimelineExporter {
public:
    /// Exports a contiguous single-video-track timeline. Clips may originate from different assets.
    /// Complex overlays, effects and transitions remain responsibilities of the future compositor.
    static bool exportContiguousVideo(const ccos::project::Project& project,
                                      const QString& outputPath,
                                      const ExportSettings& settings = {},
                                      const QString& executable = QStringLiteral("ffmpeg"),
                                      QString* error = nullptr);
};
}
