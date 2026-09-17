#pragma once

#include <cstdint>
#include <memory>
#include <vector>
#include <atomic>
#include <functional>
#include <chrono>
#include "src/timeline/TimelineModel.hpp"
#include "src/render/graph/RenderGraph.hpp"
#include "src/preview/cache/FrameCache.hpp"

namespace ccos::preview {

/**
 * @brief Configuración del pipeline de preview
 */
struct PreviewConfig {
    render::Resolution targetResolution;
    double targetFramerate;
    bool enableEffects;
    bool enableTransitions;
    bool useProxyMedia;
    int proxyQuality; // 0-100
    size_t maxCacheMemoryMB;
    bool enableAudioSync;
    bool dropFramesOnOverload;
    
    PreviewConfig()
        : targetResolution({1920, 1080})
        , targetFramerate(30.0)
        , enableEffects(true)
        , enableTransitions(true)
        , useProxyMedia(true)
        , proxyQuality(50)
        , maxCacheMemoryMB(512)
        , enableAudioSync(true)
        , dropFramesOnOverload(false) {}
};

/**
 * @brief Estado actual del preview
 */
enum class PreviewState {
    STOPPED,
    PLAYING,
    PAUSED,
    SEEKING,
    BUFFERING,
    ERROR
};

/**
 * @brief Información de estadísticas en tiempo real
 */
struct PreviewStats {
    PreviewState state;
    int64_t currentFrame;
    double currentTime; // segundos
    double framerate;
    double droppedFrameRate;
    double cacheHitRate;
    size_t cachedFrameCount;
    size_t memoryUsage;
    std::chrono::milliseconds lastFrameTime;
    bool isDroppingFrames;
    
    PreviewStats()
        : state(PreviewState::STOPPED)
        , currentFrame(0), currentTime(0.0), framerate(0.0)
        , droppedFrameRate(0.0), cacheHitRate(0.0)
        , cachedFrameCount(0), memoryUsage(0), isDroppingFrames(false) {}
};

/**
 * @brief Callback para notificación de frames renderizados
 */
using FrameReadyCallback = std::function<void(int64_t frameNumber, 
                                               const std::vector<uint8_t>& rgbaData,
                                               double timestamp)>;

/**
 * @brief Callback para notificación de errores
 */
using ErrorCallback = std::function<void(const std::string& error)>;

/**
 * @brief Callback para actualización de progreso
 */
using ProgressCallback = std::function<void(double progress, const std::string& status)>;

/**
 * @brief Pipeline principal de preview
 * 
 * Orquesta el renderizado de frames para preview en tiempo real
 * usando cache inteligente y sincronización A/V precisa
 */
class PreviewPipeline {
public:
    explicit PreviewPipeline(const PreviewConfig& config = PreviewConfig());
    ~PreviewPipeline();
    
    // No copiable, sí movible
    PreviewPipeline(const PreviewPipeline&) = delete;
    PreviewPipeline& operator=(const PreviewPipeline&) = delete;
    PreviewPipeline(PreviewPipeline&&) noexcept;
    PreviewPipeline& operator=(PreviewPipeline&&) noexcept;
    
    /**
     * @brief Inicializa el pipeline con una timeline
     * @param timeline Timeline a visualizar
     * @return true si la inicialización fue exitosa
     */
    bool initialize(const timeline::TimelineModel& timeline);
    
    /**
     * @brief Inicia reproducción desde la posición actual
     */
    void play();
    
    /**
     * @brief Pausa la reproducción
     */
    void pause();
    
    /**
     * @brief Detiene completamente la reproducción
     */
    void stop();
    
    /**
     * @brief Busca un frame específico
     * @param frameNumber Número de frame objetivo
     * @param precise Si true, espera renderizado completo
     */
    void seek(int64_t frameNumber, bool precise = true);
    
    /**
     * @brief Avanza un frame
     */
    void stepForward();
    
    /**
     * @brief Retrocede un frame
     */
    void stepBackward();
    
    /**
     * @brief Obtiene el estado actual
     */
    PreviewState getState() const;
    
    /**
     * @brief Obtiene estadísticas en tiempo real
     */
    PreviewStats getStats() const;
    
    /**
     * @brief Obtiene el frame actual (si está disponible)
     */
    std::optional<CachedFrame> getCurrentFrame() const;
    
    /**
     * @brief Establece callback para frames renderizados
     */
    void setFrameReadyCallback(FrameReadyCallback callback);
    
    /**
     * @brief Establece callback para errores
     */
    void setErrorCallback(ErrorCallback callback);
    
    /**
     * @brief Establece callback para progreso
     */
    void setProgressCallback(ProgressCallback callback);
    
    /**
     * @brief Actualiza configuración en runtime
     */
    void updateConfig(const PreviewConfig& config);
    
    /**
     * @brief Fuerza refresh del pipeline
     */
    void refresh();
    
    /**
     * @brief Limpia toda la cache
     */
    void clearCache();
    
    /**
     * @brief Verifica si hay un frame disponible para el número dado
     */
    bool isFrameAvailable(int64_t frameNumber) const;
    
    /**
     * @brief Obtiene tiempo estimado hasta que un frame esté listo
     */
    std::chrono::milliseconds getEstimatedReadyTime(int64_t frameNumber) const;

private:
    struct Impl;
    std::unique_ptr<Impl> pimpl_;
    
    void renderLoop();
    void scheduleNextFrame();
    void handleDroppedFrame();
    void updateAudioSync();
};

} // namespace ccos::preview
