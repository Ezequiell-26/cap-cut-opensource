#include "RenderScheduler.hpp"
#include <thread>
#include <chrono>
#include <algorithm>

namespace ccos::render {

// Implementación de RenderTask
RenderTask::RenderTask(std::string id) : id_(std::move(id)) {}

void RenderTask::cancel() {
    cancelled_ = true;
    state_ = RenderTaskState::Cancelled;
}

void RenderTask::pause() {
    if (state_ == RenderTaskState::Running) {
        state_ = RenderTaskState::Paused;
    }
}

void RenderTask::resume() {
    if (state_ == RenderTaskState::Paused && !cancelled_) {
        state_ = RenderTaskState::Running;
    }
}

RenderProgress RenderTask::progress() const {
    std::lock_guard<std::mutex> lock(progressMutex_);
    return progress_;
}

void RenderTask::updateProgress(double progress, int currentFrame, int totalFrames) {
    {
        std::lock_guard<std::mutex> lock(progressMutex_);
        progress_.progress = std::max(0.0, std::min(1.0, progress));
        progress_.currentFrame = currentFrame;
        progress_.totalFrames = totalFrames;
        
        // Calcular FPS y tiempo estimado
        if (progress_ fps > 0.01) {
            double remaining = (1.0 - progress_.progress) / progress_.fps;
            progress_.estimatedTimeRemaining = remaining;
        }
    }
    
    if (progressCb_) {
        progressCb_(progress_);
    }
}

void RenderTask::reportFrame(int frameNumber, void* frameData) {
    if (frameCb_) {
        frameCb_(frameNumber, frameData);
    }
}

void RenderTask::reportCompletion(const RenderResult& result) {
    state_ = result.success ? RenderTaskState::Completed : RenderTaskState::Failed;
    
    if (completionCb_) {
        completionCb_(result);
    }
}

void RenderTask::reportFailure(const std::string& error) {
    RenderResult result;
    result.success = false;
    result.errorMessage = error;
    reportCompletion(result);
}

// Implementación de RenderQueue
RenderQueue& RenderQueue::instance() {
    static RenderQueue instance;
    return instance;
}

void RenderQueue::addTask(RenderTask::Ptr task) {
    std::lock_guard<std::mutex> lock(mutex_);
    tasks_[task->id()] = task;
    
    if (autoStart_ && !running_) {
        start();
    }
}

void RenderQueue::removeTask(const std::string& taskId) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = tasks_.find(taskId);
    if (it != tasks_.end()) {
        if (it->second->state() == RenderTaskState::Running) {
            it->second->cancel();
        }
        tasks_.erase(it);
    }
}

RenderTask::Ptr RenderQueue::getTask(const std::string& taskId) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = tasks_.find(taskId);
    if (it != tasks_.end()) {
        return it->second;
    }
    return nullptr;
}

void RenderQueue::start() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (running_) return;
    
    running_ = true;
    
    // Iniciar thread de procesamiento
    std::thread([this]() {
        processQueue();
    }).detach();
}

void RenderQueue::stop() {
    std::lock_guard<std::mutex> lock(mutex_);
    running_ = false;
    
    // Cancelar todas las tareas en ejecución
    for (auto& [id, task] : tasks_) {
        if (task->state() == RenderTaskState::Running ||
            task->state() == RenderTaskState::Pending) {
            task->cancel();
        }
    }
}

void RenderQueue::pause() {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& [id, task] : tasks_) {
        if (task->state() == RenderTaskState::Running) {
            task->pause();
        }
    }
}

void RenderQueue::resume() {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& [id, task] : tasks_) {
        if (task->state() == RenderTaskState::Paused) {
            task->resume();
        }
    }
}

size_t RenderQueue::pendingCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    size_t count = 0;
    for (const auto& [id, task] : tasks_) {
        if (task->state() == RenderTaskState::Pending) {
            count++;
        }
    }
    return count;
}

size_t RenderQueue::runningCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    size_t count = 0;
    for (const auto& [id, task] : tasks_) {
        if (task->state() == RenderTaskState::Running) {
            count++;
        }
    }
    return count;
}

size_t RenderQueue::completedCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    size_t count = 0;
    for (const auto& [id, task] : tasks_) {
        if (task->state() == RenderTaskState::Completed) {
            count++;
        }
    }
    return count;
}

size_t RenderQueue::failedCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    size_t count = 0;
    for (const auto& [id, task] : tasks_) {
        if (task->state() == RenderTaskState::Failed) {
            count++;
        }
    }
    return count;
}

std::vector<RenderTask::Ptr> RenderQueue::allTasks() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<RenderTask::Ptr> result;
    for (const auto& [id, task] : tasks_) {
        result.push_back(task);
    }
    return result;
}

std::vector<RenderTask::Ptr> RenderQueue::pendingTasks() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<RenderTask::Ptr> result;
    for (const auto& [id, task] : tasks_) {
        if (task->state() == RenderTaskState::Pending) {
            result.push_back(task);
        }
    }
    return result;
}

std::vector<RenderTask::Ptr> RenderQueue::runningTasks() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<RenderTask::Ptr> result;
    for (const auto& [id, task] : tasks_) {
        if (task->state() == RenderTaskState::Running) {
            result.push_back(task);
        }
    }
    return result;
}

std::vector<RenderTask::Ptr> RenderQueue::completedTasks() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<RenderTask::Ptr> result;
    for (const auto& [id, task] : tasks_) {
        if (task->state() == RenderTaskState::Completed) {
            result.push_back(task);
        }
    }
    return result;
}

void RenderQueue::setMaxConcurrentTasks(int max) {
    std::lock_guard<std::mutex> lock(mutex_);
    maxConcurrent_ = std::max(1, max);
}

void RenderQueue::processQueue() {
    while (running_) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            
            // Contar tareas en ejecución
            int runningCount = 0;
            for (const auto& [id, task] : tasks_) {
                if (task->state() == RenderTaskState::Running) {
                    runningCount++;
                }
            }
            
            // Si hay espacio, iniciar nuevas tareas
            if (runningCount < maxConcurrent_) {
                for (auto& [id, task] : tasks_) {
                    if (task->state() == RenderTaskState::Pending && !task->isCancelled()) {
                        task->state_ = RenderTaskState::Running;
                        
                        // Ejecutar en thread separado
                        std::thread([task]() {
                            try {
                                task->execute();
                            } catch (const std::exception& e) {
                                task->reportFailure(e.what());
                            } catch (...) {
                                task->reportFailure("Unknown error occurred");
                            }
                        }).detach();
                        
                        runningCount++;
                        if (runningCount >= maxConcurrent_) {
                            break;
                        }
                    }
                }
            }
        }
        
        // Esperar antes de verificar nuevamente
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

} // namespace ccos::render
