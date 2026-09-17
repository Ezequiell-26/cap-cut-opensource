#pragma once

#include "project/Project.hpp"
#include "render/ExportSettings.hpp"

#include <QString>

#include <functional>

namespace ccos::web {

class WebFfmpegRenderer final {
public:
    using Callback = std::function<void(bool ok, const QString& message)>;

    static bool exportTimeline(const ccos::project::Project& project,
                               const ccos::render::ExportSettings& settings,
                               Callback callback);
    static void cancel();
    [[nodiscard]] static bool available() noexcept;
};

} // namespace ccos::web
