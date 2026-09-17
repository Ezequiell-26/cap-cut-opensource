#pragma once

#include <string>
#include <memory>
#include <functional>
#include <atomic>
#include <mutex>
#include <map>
#include <vector>

namespace ccos::render {

enum class RenderTaskState {
    Pending,
    Running,
    Paused,
    Completed,
    Failed,
    Cancelled
};

struct RenderProgress {
    double progress;
    int currentFrame;
    int totalFrames;
    double fps;
    double estimatedTimeRemaining;
    std::string statusMessage;

    RenderProgress()
        : progress(0.0)
        , currentFrame(0)
        , totalFrames(0)
        , fps(0.0)
        , estimatedTimeRemaining(0.0) {}
};

struct RenderResult {
    bool success;
    std::string outputPath;
    int framesRendered;
    double totalTime;
    std::string errorMessage;
    std::vector<std::string> warnings;

    RenderResult()
        : success(false)
        , framesRendered(0)
        , totalTime(0.0) {}
};

using ProgressCallback = std::function<void(const RenderProgress&)>;
using FrameCallback = std::function<void(int frameNumber, void* frameData)>;
using CompletionCallback = std::function<void(const RenderResult&)>;

struct RenderSettings {
    std::string outputFormat = "mp4";
    std::string codec = "h264";
    int quality = 23;
    int bitrate = 0;
    std::string preset = "medium";
    bool useHardwareAcceleration = false;
    std::string audioCodec = "aac";
    int audioBitrate = 192;
    int audioSampleRate = 48000;
    bool exportAudio = true;
    bool exportVideo = true;
    int startFrame = 0;
    int endFrame = -1;
    bool loopInput = false;
};

class RenderQueue;

class RenderTask {
public:
    using Ptr = std::shared_ptr<RenderTask>;

    explicit RenderTask(std::string id);
    virtual ~RenderTask() = default;

    [[nodiscard]] const std::string& id() const noexcept { return id_; }

    virtual void execute() = 0;
    virtual void cancel();
    virtual void pause();
    virtual void resume();

    [[nodiscard]] RenderTaskState state() const noexcept { return state_.load(std::memory_order_acquire); }
    [[nodiscard]] RenderProgress progress() const;
    [[nodiscard]] bool isCancelled() const noexcept { return cancelled_.load(std::memory_order_acquire); }
    [[nodiscard]] bool isCompleted() const noexcept { return state() == RenderTaskState::Completed; }
    [[nodiscard]] bool isFailed() const noexcept { return state() == RenderTaskState::Failed; }

    void setProgressCallback(ProgressCallback cb);
    void setFrameCallback(FrameCallback cb);
    void setCompletionCallback(CompletionCallback cb);

    void setSettings(const RenderSettings& settings);
    [[nodiscard]] const RenderSettings& settings() const noexcept { return settings_; }

protected:
    void updateProgress(double progress, int currentFrame, int totalFrames);
    void reportFrame(int frameNumber, void* frameData);
    void reportCompletion(const RenderResult& result);
    void reportFailure(const std::string& error);

private:
    friend class RenderQueue;

    std::string id_;
    std::atomic<RenderTaskState> state_{RenderTaskState::Pending};
    std::atomic<bool> cancelled_{false};

    RenderSettings settings_;
    RenderProgress progress_;
    mutable std::mutex progressMutex_;
    mutable std::mutex callbackMutex_;

    ProgressCallback progressCb_;
    FrameCallback frameCb_;
    CompletionCallback completionCb_;
};

class RenderQueue {
public:
    static RenderQueue& instance();

    void addTask(RenderTask::Ptr task);
    void removeTask(const std::string& taskId);
    [[nodiscard]] RenderTask::Ptr getTask(const std::string& taskId);

    void start();
    void stop();
    void pause();
    void resume();

    [[nodiscard]] size_t pendingCount() const;
    [[nodiscard]] size_t runningCount() const;
    [[nodiscard]] size_t completedCount() const;
    [[nodiscard]] size_t failedCount() const;

    [[nodiscard]] std::vector<RenderTask::Ptr> allTasks() const;
    [[nodiscard]] std::vector<RenderTask::Ptr> pendingTasks() const;
    [[nodiscard]] std::vector<RenderTask::Ptr> runningTasks() const;
    [[nodiscard]] std::vector<RenderTask::Ptr> completedTasks() const;

    void setMaxConcurrentTasks(int max);
    [[nodiscard]] int maxConcurrentTasks() const;

    void setAutoStart(bool autoStart);
    [[nodiscard]] bool autoStart() const;

private:
    RenderQueue() = default;
    ~RenderQueue() = default;
    RenderQueue(const RenderQueue&) = delete;
    RenderQueue& operator=(const RenderQueue&) = delete;

    void processQueue();

    mutable std::mutex mutex_;
    std::map<std::string, RenderTask::Ptr> tasks_;
    std::atomic<int> maxConcurrent_{1};
    std::atomic<bool> autoStart_{true};
    std::atomic<bool> running_{false};
};

} // namespace ccos::render
