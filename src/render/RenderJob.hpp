#pragma once
#include "render/ExportSettings.hpp"
#include <atomic>
#include <functional>
#include <QString>

namespace ccos::render {
enum class RenderState { Queued, Running, Completed, Failed, Cancelled };

class RenderJob {
public:
    using ProgressCallback = std::function<void(double)>;
    using MessageCallback = std::function<void(const QString&)>;

    explicit RenderJob(ExportSettings settings = {}) : settings_(std::move(settings)) {}
    void cancel() noexcept { cancelled_.store(true); }
    [[nodiscard]] bool isCancelled() const noexcept { return cancelled_.load(); }
    [[nodiscard]] RenderState state() const noexcept { return state_; }
    void setState(RenderState state) noexcept { state_ = state; }
    [[nodiscard]] const ExportSettings& settings() const noexcept { return settings_; }

private:
    ExportSettings settings_;
    std::atomic_bool cancelled_{false};
    RenderState state_ = RenderState::Queued;
};
}
