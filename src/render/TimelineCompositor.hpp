#pragma once
#include "project/Project.hpp"
#include "render/ExportSettings.hpp"
#include <QStringList>

namespace ccos::render {
class TimelineCompositor {
public:
    static bool build(const ccos::project::Project& project,
                      const ExportSettings& settings,
                      QStringList& inputs,
                      QString& filterComplex,
                      QString& videoMap,
                      QString& audioMap,
                      QString* error = nullptr);
};
}
