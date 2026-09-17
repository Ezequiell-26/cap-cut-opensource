#include "render/RenderManager.hpp"

namespace ccos::render {
std::shared_ptr<RenderJob> RenderManager::enqueue(ExportSettings settings) {
    auto job = std::make_shared<RenderJob>(std::move(settings));
    std::lock_guard lock(mutex_);
    queue_.push(job);
    return job;
}
std::shared_ptr<RenderJob> RenderManager::next() {
    std::lock_guard lock(mutex_);
    if (queue_.empty()) return {};
    auto job = queue_.front();
    queue_.pop();
    return job;
}
std::size_t RenderManager::pendingCount() const {
    std::lock_guard lock(mutex_);
    return queue_.size();
}
}
