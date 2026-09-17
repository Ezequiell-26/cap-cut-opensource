#pragma once

#include <string>
#include <memory>
#include <functional>
#include <atomic>
#include <mutex>
#include <map>

namespace ccos::render {

// Estado de una tarea de render
enum class RenderTaskState {
    Pending,
    Running,
    Paused,
    Completed,
    Failed,
    Cancelled
};

// Progreso del render
struct RenderProgress {
    double progress;      // 0.0 a 1.0
    int currentFrame;
    int totalFrames;
    double fps;           // Frames por segundo actuales
    double estimatedTimeRemaining; // Segundos
    std::string statusMessage;
    
    RenderProgress() 
        : progress(0.0), currentFrame(0), totalFrames(0), 
          fps(0.0), estimatedTimeRemaining(0.0) {}
};

// Resultado de una tarea de render
struct RenderResult {
    bool success;
    std::string outputPath;
    int framesRendered;
    double totalTime;
    std::string errorMessage;
    std::vector<std::string> warnings;
    
    RenderResult() 
        : success(false), framesRendered(0), totalTime(0.0) {}
};

// Callbacks para seguimiento del render
using ProgressCallback = std::function<void(const RenderProgress&)>;
using FrameCallback = std::function<void(int frameNumber, void* frameData)>;
using CompletionCallback = std::function<void(const RenderResult&)>;

// Configuración de render
struct RenderSettings {
    std::string outputFormat = "mp4";
    std::string codec = "h264";
    int quality = 23;              // CRF para x264/x265
    int bitrate = 0;               // 0 = variable quality
    std::string preset = "medium"; // Velocidad vs compresión
    bool useHardwareAcceleration = false;
    std::string audioCodec = "aac";
    int audioBitrate = 192;
    int audioSampleRate = 48000;
    bool exportAudio = true;
    bool exportVideo = true;
    int startFrame = 0;
    int endFrame = -1;             // -1 = hasta el final
    bool loopInput = false;
};

// Tarea de render individual
class RenderTask {
public:
    using Ptr = std::shared_ptr<RenderTask>;
    
    explicit RenderTask(std::string id);
    virtual ~RenderTask() = default;
    
    const std::string& id() const { return id_; }
    
    // Control de ejecución
    virtual void execute() = 0;
    virtual void cancel();
    virtual void pause();
    virtual void resume();
    
    // Estado
    RenderTaskState state() const { return state_; }
    RenderProgress progress() const;
    bool isCancelled() const { return cancelled_; }
    bool isCompleted() const { return state_ == RenderTaskState::Completed; }
    bool isFailed() const { return state_ == RenderTaskState::Failed; }
    
    // Callbacks
    void setProgressCallback(ProgressCallback cb) { progressCb_ = std::move(cb); }
    void setFrameCallback(FrameCallback cb) { frameCb_ = std::move(cb); }
    void setCompletionCallback(CompletionCallback cb) { completionCb_ = std::move(cb); }
    
    // Configuración
    void setSettings(const RenderSettings& settings) { settings_ = settings; }
    const RenderSettings& settings() const { return settings_; }
    
protected:
    void updateProgress(double progress, int currentFrame, int totalFrames);
    void reportFrame(int frameNumber, void* frameData);
    void reportCompletion(const RenderResult& result);
    void reportFailure(const std::string& error);
    
    std::string id_;
    std::atomic<RenderTaskState> state_{RenderTaskState::Pending};
    std::atomic<bool> cancelled_{false};
    
    RenderSettings settings_;
    RenderProgress progress_;
    
    mutable std::mutex progressMutex_;
    
    ProgressCallback progressCb_;
    FrameCallback frameCb_;
    CompletionCallback completionCb_;
};

// Cola de render para múltiples tareas
class RenderQueue {
public:
    static RenderQueue& instance();
    
    // Gestión de tareas
    void addTask(RenderTask::Ptr task);
    void removeTask(const std::string& taskId);
    RenderTask::Ptr getTask(const std::string& taskId);
    
    // Control de cola
    void start();
    void stop();
    void pause();
    void resume();
    
    // Estado de la cola
    size_t pendingCount() const;
    size_t runningCount() const;
    size_t completedCount() const;
    size_t failedCount() const;
    
    std::vector<RenderTask::Ptr> allTasks() const;
    std::vector<RenderTask::Ptr> pendingTasks() const;
    std::vector<RenderTask::Ptr> runningTasks() const;
    std::vector<RenderTask::Ptr> completedTasks() const;
    
    // Configuración global
    void setMaxConcurrentTasks(int max);
    int maxConcurrentTasks() const { return maxConcurrent_; }
    
    void setAutoStart(bool autoStart) { autoStart_ = autoStart; }
    bool autoStart() const { return autoStart_; }
    
private:
    RenderQueue() = default;
    ~RenderQueue() = default;
    RenderQueue(const RenderQueue&) = delete;
    RenderQueue& operator=(const RenderQueue&) = delete;
    
    void processQueue();
    
    mutable std::mutex mutex_;
    std::map<std::string, RenderTask::Ptr> tasks_;
    
    int maxConcurrent_ = 1;
    bool autoStart_ = true;
    bool running_ = false;
};

} // namespace ccos::render
