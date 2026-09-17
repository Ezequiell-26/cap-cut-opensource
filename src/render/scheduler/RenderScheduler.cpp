#include "RenderScheduler.hpp"

#include <algorithm>
#include <chrono>
#include <exception>
#include <thread>
#include <utility>

namespace ccos::render {

RenderTask::RenderTask(std::string id)
    : id_(std::move(id)) {}

void RenderTask::cancel() {
    cancelled_.store(true, std::memory_order_release);
    state_.store(RenderTaskState::Cancelled, std::memory_order_release);
}

void RenderTask::pause() {
    RenderTaskState expected = RenderTaskState::Running;
    state_.compare_exchange_strong(expected, RenderTaskState::Paused, std::memory_order_acq_rel);
}

void RenderTask::resume() {
    if (!cancelled_.load(std::memory_order_acquire)) {
        RenderTaskState expected = RenderTaskState::Paused;
        state_.compare_exchange_strong(expected, RenderTaskState::Running, std::memory_order_acq_rel);
    }
}

RenderProgress RenderTask::progress() const {
    std::lock_guard<std::mutex> lock(progressMutex_);
    return progress_;
}

void RenderTask::updateProgress(double progress, int currentFrame, int totalFrames) {
    ProgressCallback callback;
    RenderProgress snapshot;
    {
        std::lock_guard<std::mutex> lock(progressMutex_);
        progress_.progress = std::clamp(progress, 0.0, 1.0);
        progress_.currentFrame = std::max(0, currentFrame);
        progress_.totalFrames = std::max(0, totalFrames);
        if (progress_.fps > 0.01) {
            progress_.estimatedTimeRemaining = (1.0 - progress_.progress) / progress_.fps;
        }
        snapshot = progress_;
    }
    {
        std::lock_guard<std::mutex> lock(callbackMutex_);
        callback = progressCb_;
    }
    if (callback) callback(snapshot);
}

void RenderTask::reportFrame(int frameNumber, void* frameData) {
    FrameCallback callback;
    {
        std::lock_guard<std::mutex> lock(callbackMutex_);
        callback = frameCb_;
    }
    if (callback) callback(frameNumber, frameData);
}

void RenderTask::reportCompletion(const RenderResult& result) {
    if (cancelled_.load(std::memory_order_acquire)) {
        state_.store(RenderTaskState::Cancelled, std::memory_order_release);
    } else {
        state_.store(result.success ? RenderTaskState::Completed : RenderTaskState::Failed,
                     std::memory_order_release);
    }

    CompletionCallback callback;
    {
        std::lock_guard<std::mutex> lock(callbackMutex_);
        callback = completionCb_;
    }
    if (callback) callback(result);
}

void RenderTask::reportFailure(const std::string& error) {
    RenderResult result;
    result.success = false;
    result.errorMessage = error;
    reportCompletion(result);
}

void RenderTask::setProgressCallback(ProgressCallback cb) {
    std::lock_guard<std::mutex> lock(callbackMutex_);
    progressCb_ = std::move(cb);
}

void RenderTask::setFrameCallback(FrameCallback cb) {
    std::lock_guard<std::mutex> lock(callbackMutex_);
    frameCb_ = std::move(cb);
}

void RenderTask::setCompletionCallback(CompletionCallback cb) {
    std::lock_guard<std::mutex> lock(callbackMutex_);
    completionCb_ = std::move(cb);
}

void RenderTask::setSettings(const RenderSettings& settings) {
    std::lock_guard<std::mutex> lock(progressMutex_);
    settings_ = settings;
}

RenderQueue& RenderQueue::instance() {
    static RenderQueue instance;
    return instance;
}

void RenderQueue::addTask(RenderTask::Ptr task) {
    if (!task || task->id().empty()) return;

    bool shouldStart = false;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        tasks_[task->id()] = std::move(task);
        shouldStart = autoStart_.load(std::memory_order_acquire) &&
                      !running_.load(std::memory_order_acquire);
    }
    if (shouldStart) start();
}

void RenderQueue::removeTask(const std::string& taskId) {
    RenderTask::Ptr task;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        const auto it = tasks_.find(taskId);
        if (it == tasks_.end()) return;
        task = it->second;
        tasks_.erase(it);
    }
    if (task && task->state() == RenderTaskState::Running) task->cancel();
}

RenderTask::Ptr RenderQueue::getTask(const std::string& taskId) {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto it = tasks_.find(taskId);
    return it == tasks_.end() ? nullptr : it->second;
}

void RenderQueue::start() {
    bool expected = false;
    if (!running_.compare_exchange_strong(expected, true, std::memory_order_acq_rel)) return;
    std::thread(&RenderQueue::processQueue, this).detach();
}

void RenderQueue::stop() {
    running_.store(false, std::memory_order_release);
    std::vector<RenderTask::Ptr> snapshot;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& [id, task] : tasks_) snapshot.push_back(task);
    }
    for (const auto& task : snapshot) {
        if (task && (task->state() == RenderTaskState::Running || task->state() == RenderTaskState::Pending ||
                     task->state() == RenderTaskState::Paused)) {
            task->cancel();
        }
    }
}

void RenderQueue::pause() {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& [id, task] : tasks_) {
        if (task) task->pause();
    }
}

void RenderQueue::resume() {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& [id, task] : tasks_) {
        if (task) task->resume();
    }
}

size_t RenderQueue::pendingCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    size_t count = 0;
    for (const auto& [id, task] : tasks_) if (task && task->state() == RenderTaskState::Pending) ++count;
    return count;
}

size_t RenderQueue::runningCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    size_t count = 0;
    for (const auto& [id, task] : tasks_) if (task && task->state() == RenderTaskState::Running) ++count;
    return count;
}

size_t RenderQueue::completedCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    size_t count = 0;
    for (const auto& [id, task] : tasks_) if (task && task->state() == RenderTaskState::Completed) ++count;
    return count;
}

size_t RenderQueue::failedCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    size_t count = 0;
    for (const auto& [id, task] : tasks_) if (task && task->state() == RenderTaskState::Failed) ++count;
    return count;
}

std::vector<RenderTask::Ptr> RenderQueue::allTasks() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<RenderTask::Ptr> result;
    result.reserve(tasks_.size());
    for (const auto& [id, task] : tasks_) result.push_back(task);
    return result;
}

std::vector<RenderTask::Ptr> RenderQueue::pendingTasks() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<RenderTask::Ptr> result;
    for (const auto& [id, task] : tasks_) if (task && task->state() == RenderTaskState::Pending) result.push_back(task);
    return result;
}

std::vector<RenderTask::Ptr> RenderQueue::runningTasks() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<RenderTask::Ptr> result;
    for (const auto& [id, task] : tasks_) if (task && task->state() == RenderTaskState::Running) result.push_back(task);
    return result;
}

std::vector<RenderTask::Ptr> RenderQueue::completedTasks() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<RenderTask::Ptr> result;
    for (const auto& [id, task] : tasks_) if (task && task->state() == RenderTaskState::Completed) result.push_back(task);
    return result;
}

void RenderQueue::setMaxConcurrentTasks(int max) {
    maxConcurrent_.store(std::max(1, max), std::memory_order_release);
}

int RenderQueue::maxConcurrentTasks() const {
    return maxConcurrent_.load(std::memory_order_acquire);
}

void RenderQueue::setAutoStart(bool autoStart) {
    autoStart_.store(autoStart, std::memory_order_release);
}

bool RenderQueue::autoStart() const {
    return autoStart_.load(std::memory_order_acquire);
}

void RenderQueue::processQueue() {
    while (running_.load(std::memory_order_acquire)) {
        std::vector<RenderTask::Ptr> toRun;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            int active = 0;
            for (const auto& [id, task] : tasks_) {
                if (task && task->state() == RenderTaskState::Running) ++active;
            }

            const int limit = maxConcurrent_.load(std::memory_order_acquire);
            for (const auto& [id, task] : tasks_) {
                if (!task || task->isCancelled() || task->state() != RenderTaskState::Pending) continue;
                if (active >= limit) break;
                task->state_.store(RenderTaskState::Running, std::memory_order_release);
                toRun.push_back(task);
                ++active;
            }
        }

        for (const auto& task : toRun) {
            std::thread([task]() {
                try {
                    task->execute();
                } catch (const std::exception& exception) {
                    task->reportFailure(exception.what());
                } catch (...) {
                    task->reportFailure("Unknown render task error");
                }
            }).detach();
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(25));
    }
}

} // namespace ccos::render
