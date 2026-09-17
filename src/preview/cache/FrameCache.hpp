#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <chrono>
#include <mutex>
#include <atomic>
#include "src/render/graph/RenderGraphTypes.hpp"

namespace ccos::preview {

/**
 * @brief Representación de un frame en cache
 */
struct CachedFrame {
    int64_t frameNumber;
    render::Resolution resolution;
    std::vector<uint8_t> data; // RGBA
    std::chrono::steady_clock::time_point timestamp;
    size_t memoryUsage;
    bool isKeyframe;
    
    CachedFrame() : frameNumber(0), memoryUsage(0), isKeyframe(false) {}
};

/**
 * @brief Política de reemplazo para cache LRU
 */
enum class CacheEvictionPolicy {
    LRU,        // Least Recently Used
    LFU,        // Least Frequently Used
    FIFO,       // First In First Out
    ADAPTIVE    // Adaptive replacement
};

/**
 * @brief Configuración del sistema de cache
 */
struct CacheConfig {
    size_t maxMemoryBytes;
    size_t maxFrames;
    CacheEvictionPolicy evictionPolicy;
    bool enableCompression;
    bool enableDiskCache;
    std::string diskCachePath;
    int compressionLevel; // 0-9
    
    CacheConfig() 
        : maxMemoryBytes(512 * 1024 * 1024) // 512MB default
        , maxFrames(1000)
        , evictionPolicy(CacheEvictionPolicy::LRU)
        , enableCompression(false)
        , enableDiskCache(false)
        , compressionLevel(6) {}
};

/**
 * @brief Estadísticas de uso de cache
 */
struct CacheStats {
    size_t totalHits;
    size_t totalMisses;
    size_t currentFrameCount;
    size_t currentMemoryUsage;
    size_t evictions;
    size_t diskWrites;
    size_t diskReads;
    
    double hitRate() const {
        size_t total = totalHits + totalMisses;
        return total > 0 ? static_cast<double>(totalHits) / total : 0.0;
    }
    
    CacheStats() 
        : totalHits(0), totalMisses(0), currentFrameCount(0)
        , currentMemoryUsage(0), evictions(0), diskWrites(0), diskReads(0) {}
};

/**
 * @brief Sistema de cache de frames para preview
 * 
 * Maneja almacenamiento inteligente de frames renderizados
 * con políticas de reemplazo y soporte para disco
 */
class FrameCache {
public:
    explicit FrameCache(const CacheConfig& config = CacheConfig());
    ~FrameCache();
    
    // No copiable, sí movible
    FrameCache(const FrameCache&) = delete;
    FrameCache& operator=(const FrameCache&) = delete;
    FrameCache(FrameCache&&) noexcept;
    FrameCache& operator=(FrameCache&&) noexcept;
    
    /**
     * @brief Intenta obtener un frame de la cache
     * @param frameNumber Número de frame
     * @return Frame cacheado si existe, nulo si miss
     */
    std::optional<CachedFrame> get(int64_t frameNumber);
    
    /**
     * @brief Almacena un frame en la cache
     * @param frame Frame a cache
     * @return true si se almacenó exitosamente
     */
    bool put(const CachedFrame& frame);
    
    /**
     * @brief Verifica si un frame está en cache
     */
    bool contains(int64_t frameNumber) const;
    
    /**
     * @brief Elimina un frame específico
     */
    void remove(int64_t frameNumber);
    
    /**
     * @brief Limpia toda la cache
     */
    void clear();
    
    /**
     * @brief Reserva espacio para frames específicos
     */
    void reserve(const std::vector<int64_t>& frameNumbers);
    
    /**
     * @brief Obtiene estadísticas de uso
     */
    CacheStats getStats() const;
    
    /**
     * @brief Actualiza configuración en runtime
     */
    void updateConfig(const CacheConfig& config);
    
    /**
     * @brief Fuerza escritura a disco de frames importantes
     */
    void flushToDisk();
    
    /**
     * @brief Precarga frames adyacentes (prefetch)
     */
    void prefetchAround(int64_t currentFrame, int range);

private:
    struct Impl;
    std::unique_ptr<Impl> pimpl_;
    mutable std::mutex mutex_;
    
    void evictIfNeeded();
    void updateAccessTime(int64_t frameNumber);
    size_t calculateMemoryUsage(const CachedFrame& frame) const;
};

} // namespace ccos::preview
