#include "PreviewPipeline.hpp"
#include <thread>
#include <queue>
#include <condition_variable>
#include <mutex>
#include <atomic>

namespace ccos::preview {

struct PreviewPipeline::Impl {
    PreviewConfig config;
    PreviewStats stats;
    
    // Timeline actual
    const timeline::TimelineModel* timeline = nullptr;
    
    // Cache de frames
    FrameCache frameCache;
    
    // Estado de reproducción
    std::atomic<PreviewState> state{PreviewState::STOPPED};
    std::atomic<int64_t> currentFrame{0};
    std::atomic<bool> shouldStop{false};
    
    // Hilo de renderizado
    std::thread renderThread;
    std::mutex renderMutex;
    std::condition_variable renderCV;
    
    // Cola de frames pendientes
    std::queue<int64_t> pendingFrames;
    std::mutex queueMutex;
    
    // Callbacks
    FrameReadyCallback frameReadyCallback;
    ErrorCallback errorCallback;
    ProgressCallback progressCallback;
    
    // Timing
    std::chrono::steady_clock::time_point lastFrameTime;
    std::atomic<int64_t> totalFramesRendered{0};
    std::atomic<int64_t> droppedFrames{0};
    
    // Sincronización A/V
    double audioClock = 0.0;
    double videoClock = 0.0;
    std::mutex clockMutex;
    
    Impl(const PreviewConfig& cfg) 
        : config(cfg)
        , frameCache([&]() {
            CacheConfig cacheCfg;
            cacheCfg.maxMemoryBytes = cfg.maxCacheMemoryMB * 1024 * 1024;
            return cacheCfg;
          }()) {}
};

PreviewPipeline::PreviewPipeline(const PreviewConfig& config)
    : pimpl_(std::make_unique<Impl>(config)) {}

PreviewPipeline::~PreviewPipeline() {
    stop();
    if (pimpl_->renderThread.joinable()) {
        pimpl_->shouldStop = true;
        pimpl_->renderCV.notify_all();
        pimpl_->renderThread.join();
    }
}

PreviewPipeline::PreviewPipeline(PreviewPipeline&&) noexcept = default;
PreviewPipeline& PreviewPipeline::operator=(PreviewPipeline&&) noexcept = default;

bool PreviewPipeline::initialize(const timeline::TimelineModel& timeline) {
    if (!timeline.isValid()) {
        if (pimpl_->errorCallback) {
            pimpl_->errorCallback("Timeline inválida");
        }
        return false;
    }
    
    pimpl_->timeline = &timeline;
    pimpl_->currentFrame = 0;
    pimpl_->stats.state = PreviewState::STOPPED;
    
    // Construir grafo de render para preview
    // Esto se integrará con el RenderGraph del motor de render
    
    return true;
}

void PreviewPipeline::play() {
    if (pimpl_->state == PreviewState::PLAYING) {
        return;
    }
    
    pimpl_->state = PreviewState::PLAYING;
    pimpl_->shouldStop = false;
    pimpl_->lastFrameTime = std::chrono::steady_clock::now();
    
    // Iniciar hilo de renderizado si no está activo
    if (!pimpl_->renderThread.joinable()) {
        pimpl_->renderThread = std::thread(&PreviewPipeline::renderLoop, this);
    }
    
    pimpl_->renderCV.notify_all();
}

void PreviewPipeline::pause() {
    if (pimpl_->state != PreviewState::PLAYING) {
        return;
    }
    
    pimpl_->state = PreviewState::PAUSED;
    pimpl_->renderCV.notify_all();
}

void PreviewPipeline::stop() {
    pimpl_->state = PreviewState::STOPPED;
    pimpl_->shouldStop = true;
    pimpl_->renderCV.notify_all();
    
    if (pimpl_->renderThread.joinable()) {
        pimpl_->renderThread.join();
    }
}

void PreviewPipeline::seek(int64_t frameNumber, bool precise) {
    std::lock_guard<std::mutex> lock(pimpl_->renderMutex);
    
    pimpl_->state = PreviewState::SEEKING;
    
    // Verificar si el frame ya está en cache
    auto cachedFrame = pimpl_->frameCache.get(frameNumber);
    if (cachedFrame && !precise) {
        pimpl_->currentFrame = frameNumber;
        pimpl_->state = PreviewState::PAUSED;
        
        if (pimpl_->frameReadyCallback) {
            pimpl_->frameReadyCallback(
                cachedFrame->frameNumber,
                cachedFrame->data,
                static_cast<double>(frameNumber) / pimpl_->config.targetFramerate
            );
        }
        return;
    }
    
    // Agregar a cola de prioridad
    pimpl_->pendingFrames.push(frameNumber);
    pimpl_->renderCV.notify_all();
    
    // Esperar si es preciso
    if (precise) {
        // Implementación simplificada - en producción usar condition_variable
        pimpl_->state = PreviewState::BUFFERING;
    }
}

void PreviewPipeline::stepForward() {
    seek(pimpl_->currentFrame + 1, true);
}

void PreviewPipeline::stepBackward() {
    seek(std::max(int64_t(0), pimpl_->currentFrame - 1), true);
}

PreviewState PreviewPipeline::getState() const {
    return pimpl_->state;
}

PreviewStats PreviewPipeline::getStats() const {
    PreviewStats stats = pimpl_->stats;
    stats.state = pimpl_->state;
    stats.currentFrame = pimpl_->currentFrame;
    
    auto cacheStats = pimpl_->frameCache.getStats();
    stats.cacheHitRate = cacheStats.hitRate();
    stats.cachedFrameCount = cacheStats.currentFrameCount;
    stats.memoryUsage = cacheStats.currentMemoryUsage;
    
    int64_t total = pimpl_->totalFramesRendered + pimpl_->droppedFrames;
    stats.droppedFrameRate = total > 0 
        ? static_cast<double>(pimpl_->droppedFrames) / total 
        : 0.0;
    
    stats.isDroppingFrames = stats.droppedFrameRate > 0.1; // Más del 10%
    
    return stats;
}

std::optional<CachedFrame> PreviewPipeline::getCurrentFrame() const {
    return pimpl_->frameCache.get(pimpl_->currentFrame);
}

void PreviewPipeline::setFrameReadyCallback(FrameReadyCallback callback) {
    pimpl_->frameReadyCallback = callback;
}

void PreviewPipeline::setErrorCallback(ErrorCallback callback) {
    pimpl_->errorCallback = callback;
}

void PreviewPipeline::setProgressCallback(ProgressCallback callback) {
    pimpl_->progressCallback = callback;
}

void PreviewPipeline::updateConfig(const PreviewConfig& config) {
    pimpl_->config = config;
    
    CacheConfig cacheCfg;
    cacheCfg.maxMemoryBytes = config.maxCacheMemoryMB * 1024 * 1024;
    pimpl_->frameCache.updateConfig(cacheCfg);
}

void PreviewPipeline::refresh() {
    std::lock_guard<std::mutex> lock(pimpl_->renderMutex);
    pimpl_->frameCache.clear();
    pimpl_->renderCV.notify_all();
}

void PreviewPipeline::clearCache() {
    pimpl_->frameCache.clear();
}

bool PreviewPipeline::isFrameAvailable(int64_t frameNumber) const {
    return pimpl_->frameCache.contains(frameNumber);
}

std::chrono::milliseconds PreviewPipeline::getEstimatedReadyTime(int64_t frameNumber) const {
    if (pimpl_->frameCache.contains(frameNumber)) {
        return std::chrono::milliseconds(0);
    }
    
    // Estimación basada en frames pendientes y tiempo promedio de render
    size_t pendingCount = pimpl_->pendingFrames.size();
    int64_t avgRenderTime = 33; // 33ms por frame (~30fps)
    
    return std::chrono::milliseconds(pendingCount * avgRenderTime);
}

void PreviewPipeline::renderLoop() {
    while (!pimpl_->shouldStop) {
        std::unique_lock<std::mutex> lock(pimpl_->renderMutex);
        
        // Esperar si está pausado o sin frames pendientes
        pimpl_->renderCV.wait(lock, [this]() {
            return pimpl_->shouldStop || 
                   pimpl_->state == PreviewState::PLAYING ||
                   pimpl_->state == PreviewState::SEEKING ||
                   pimpl_->state == PreviewState::BUFFERING ||
                   !pimpl_->pendingFrames.empty();
        });
        
        if (pimpl_->shouldStop) {
            break;
        }
        
        // Obtener siguiente frame a renderizar
        int64_t frameToRender = -1;
        
        if (!pimpl_->pendingFrames.empty()) {
            frameToRender = pimpl_->pendingFrames.front();
            pimpl_->pendingFrames.pop();
        } else if (pimpl_->state == PreviewState::PLAYING) {
            frameToRender = pimpl_->currentFrame + 1;
        }
        
        if (frameToRender < 0) {
            continue;
        }
        
        lock.unlock();
        
        // Verificar cache primero
        if (pimpl_->frameCache.contains(frameToRender)) {
            auto cachedFrame = pimpl_->frameCache.get(frameToRender);
            if (cachedFrame) {
                pimpl_->currentFrame = frameToRender;
                
                if (pimpl_->frameReadyCallback) {
                    pimpl_->frameReadyCallback(
                        cachedFrame->frameNumber,
                        cachedFrame->data,
                        static_cast<double>(frameToRender) / pimpl_->config.targetFramerate
                    );
                }
                
                pimpl_->totalFramesRendered++;
                continue;
            }
        }
        
        // Renderizar frame (implementación simplificada)
        // En producción, esto construiría y ejecutaría el RenderGraph
        
        // Simular renderizado
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
        
        // Crear frame dummy
        CachedFrame renderedFrame;
        renderedFrame.frameNumber = frameToRender;
        renderedFrame.resolution = pimpl_->config.targetResolution;
        
        size_t frameSize = renderedFrame.resolution.width * 
                          renderedFrame.resolution.height * 4; // RGBA
        renderedFrame.data.resize(frameSize, 0);
        renderedFrame.timestamp = std::chrono::steady_clock::now();
        renderedFrame.memoryUsage = frameSize;
        
        // Guardar en cache
        pimpl_->frameCache.put(renderedFrame);
        pimpl_->currentFrame = frameToRender;
        pimpl_->totalFramesRendered++;
        
        // Notificar callback
        if (pimpl_->frameReadyCallback) {
            pimpl_->frameReadyCallback(
                renderedFrame.frameNumber,
                renderedFrame.data,
                static_cast<double>(frameToRender) / pimpl_->config.targetFramerate
            );
        }
        
        // Actualizar timing
        auto now = std::chrono::steady_clock::now();
        pimpl_->stats.lastFrameTime = 
            std::chrono::duration_cast<std::chrono::milliseconds>(now - pimpl_->lastFrameTime);
        pimpl_->lastFrameTime = now;
        
        // Verificar si debemos dropping frames
        if (pimpl_->config.dropFramesOnOverload) {
            auto frameTime = pimpl_->stats.lastFrameTime.count();
            double targetFrameTime = 1000.0 / pimpl_->config.targetFramerate;
            
            if (frameTime > targetFrameTime * 1.5) {
                handleDroppedFrame();
            }
        }
        
        // Actualizar sincronización A/V
        updateAudioSync();
    }
}

void PreviewPipeline::scheduleNextFrame() {
    if (pimpl_->state == PreviewState::PLAYING) {
        int64_t nextFrame = pimpl_->currentFrame + 1;
        pimpl_->pendingFrames.push(nextFrame);
        
        // Prefetch de frames adyacentes
        if (!pimpl_->frameCache.contains(nextFrame + 1)) {
            pimpl_->pendingFrames.push(nextFrame + 1);
        }
    }
    
    pimpl_->renderCV.notify_one();
}

void PreviewPipeline::handleDroppedFrame() {
    pimpl_->droppedFrames++;
    
    if (pimpl_->progressCallback) {
        pimpl_->progressCallback(0.0, "Dropping frames por sobrecarga");
    }
}

void PreviewPipeline::updateAudioSync() {
    std::lock_guard<std::mutex> lock(pimpl_->clockMutex);
    
    if (!pimpl_->config.enableAudioSync) {
        return;
    }
    
    // Actualizar reloj de video
    pimpl_->videoClock = static_cast<double>(pimpl_->currentFrame) / 
                         pimpl_->config.targetFramerate;
    
    // Calcular diferencia con audio clock
    double diff = pimpl_->videoClock - pimpl_->audioClock;
    
    // Si la diferencia es muy grande, ajustar
    if (std::abs(diff) > 0.1) { // Más de 100ms
        // En producción, ajustar velocidad de reproducción o hacer seek
        if (pimpl_->progressCallback) {
            pimpl_->progressCallback(0.0, "A/V sync ajustado");
        }
    }
}

} // namespace ccos::preview
