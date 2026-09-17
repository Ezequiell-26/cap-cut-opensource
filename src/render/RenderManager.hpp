#pragma once
#include "render/RenderJob.hpp"
#include <memory>
#include <mutex>
#include <queue>

namespace ccos::render {
class RenderManager {
public:
    std::shared_ptr<RenderJob> enqueue(ExportSettings settings = {});
    std::shared_ptr<RenderJob> next();
    [[nodiscard]] std::size_t pendingCount() const;
private:
    mutable std::mutex mutex_;
    std::queue<std::shared_ptr<RenderJob>> queue_;
};
}
