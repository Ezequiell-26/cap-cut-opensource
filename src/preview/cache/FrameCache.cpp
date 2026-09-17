#include "FrameCache.hpp"
#include <unordered_map>
#include <map>
#include <queue>
#include <algorithm>
#include <fstream>
#include <filesystem>

namespace ccos::preview {

struct FrameCache::Impl {
    CacheConfig config;
    CacheStats stats;
    
    // Almacenamiento principal de frames
    std::unordered_map<int64_t, CachedFrame> frames;
    
    // Tracking de accesos para LRU (timestamp del último acceso)
    std::unordered_map<int64_t, std::chrono::steady_clock::time_point> accessTimes;
    
    // Tracking de frecuencia para LFU
    std::unordered_map<int64_t, size_t> accessCounts;
    
    // Cola FIFO para política FIFO
    std::queue<int64_t> fifoQueue;
    
    // Frames marcados como importantes (no evictar fácilmente)
    std::unordered_map<int64_t, bool> importantFrames;
    
    // Cache en disco
    std::unordered_map<int64_t, std::string> diskLocations;
    
    Impl(const CacheConfig& cfg) : config(cfg) {
        if (config.enableDiskCache && !config.diskCachePath.empty()) {
            std::filesystem::create_directories(config.diskCachePath);
        }
    }
};

FrameCache::FrameCache(const CacheConfig& config) 
    : pimpl_(std::make_unique<Impl>(config)) {}

FrameCache::~FrameCache() = default;

FrameCache::FrameCache(FrameCache&&) noexcept = default;
FrameCache& FrameCache::operator=(FrameCache&&) noexcept = default;

std::optional<CachedFrame> FrameCache::get(int64_t frameNumber) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = pimpl_->frames.find(frameNumber);
    if (it != pimpl_->frames.end()) {
        // Hit
        pimpl_->stats.totalHits++;
        updateAccessTime(frameNumber);
        
        // Incrementar contador de frecuencia
        pimpl_->accessCounts[frameNumber]++;
        
        return it->second;
    }
    
    // Miss - verificar disco si está habilitado
    if (pimpl_->config.enableDiskCache) {
        auto diskIt = pimpl_->diskLocations.find(frameNumber);
        if (diskIt != pimpl_->diskLocations.end()) {
            // Leer desde disco
            std::ifstream file(diskIt->second, std::ios::binary);
            if (file.is_open()) {
                CachedFrame frame;
                file.read(reinterpret_cast<char*>(&frame.frameNumber), sizeof(frame.frameNumber));
                file.read(reinterpret_cast<char*>(&frame.resolution.width), sizeof(frame.resolution.width));
                file.read(reinterpret_cast<char*>(&frame.resolution.height), sizeof(frame.resolution.height));
                
                size_t dataSize;
                file.read(reinterpret_cast<char*>(&dataSize), sizeof(dataSize));
                frame.data.resize(dataSize);
                file.read(reinterpret_cast<char*>(frame.data.data()), dataSize);
                
                pimpl_->stats.diskReads++;
                pimpl_->stats.totalHits++;
                
                // Mover a memoria
                put(frame);
                return frame;
            }
        }
    }
    
    pimpl_->stats.totalMisses++;
    return std::nullopt;
}

bool FrameCache::put(const CachedFrame& frame) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Verificar si ya existe
    if (pimpl_->frames.count(frame.frameNumber)) {
        pimpl_->frames[frame.frameNumber] = frame;
        updateAccessTime(frame.frameNumber);
        return true;
    }
    
    // Evictar si es necesario antes de agregar
    evictIfNeeded();
    
    // Calcular uso de memoria
    size_t memUsage = calculateMemoryUsage(frame);
    
    // Verificar límites
    if (pimpl_->frames.size() >= pimpl_->config.maxFrames ||
        pimpl_->stats.currentMemoryUsage + memUsage > pimpl_->config.maxMemoryBytes) {
        // No se puede agregar más
        return false;
    }
    
    // Agregar frame
    pimpl_->frames[frame.frameNumber] = frame;
    pimpl_->stats.currentFrameCount = pimpl_->frames.size();
    pimpl_->stats.currentMemoryUsage += memUsage;
    
    // Inicializar tracking
    pimpl_->accessTimes[frame.frameNumber] = std::chrono::steady_clock::now();
    pimpl_->accessCounts[frame.frameNumber] = 1;
    pimpl_->fifoQueue.push(frame.frameNumber);
    
    return true;
}

bool FrameCache::contains(int64_t frameNumber) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return pimpl_->frames.count(frameNumber) > 0;
}

void FrameCache::remove(int64_t frameNumber) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = pimpl_->frames.find(frameNumber);
    if (it != pimpl_->frames.end()) {
        size_t memUsage = calculateMemoryUsage(it->second);
        pimpl_->frames.erase(it);
        pimpl_->stats.currentMemoryUsage -= memUsage;
        pimpl_->stats.currentFrameCount = pimpl_->frames.size();
        
        // Limpiar tracking
        pimpl_->accessTimes.erase(frameNumber);
        pimpl_->accessCounts.erase(frameNumber);
        pimpl_->importantFrames.erase(frameNumber);
        
        // Eliminar de disco si existe
        if (pimpl_->config.enableDiskCache) {
            auto diskIt = pimpl_->diskLocations.find(frameNumber);
            if (diskIt != pimpl_->diskLocations.end()) {
                std::filesystem::remove(diskIt->second);
                pimpl_->diskLocations.erase(diskIt);
            }
        }
    }
}

void FrameCache::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    pimpl_->frames.clear();
    pimpl_->accessTimes.clear();
    pimpl_->accessCounts.clear();
    
    // Limpiar cola FIFO
    while (!pimpl_->fifoQueue.empty()) {
        pimpl_->fifoQueue.pop();
    }
    
    pimpl_->importantFrames.clear();
    pimpl_->stats.currentFrameCount = 0;
    pimpl_->stats.currentMemoryUsage = 0;
    
    // Limpiar disco
    if (pimpl_->config.enableDiskCache && !pimpl_->config.diskCachePath.empty()) {
        for (const auto& [frameNum, path] : pimpl_->diskLocations) {
            std::filesystem::remove(path);
        }
        pimpl_->diskLocations.clear();
    }
}

void FrameCache::reserve(const std::vector<int64_t>& frameNumbers) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    for (int64_t num : frameNumbers) {
        if (!pimpl_->frames.count(num)) {
            // Marcar como importante para que no sea evictado fácilmente
            pimpl_->importantFrames[num] = true;
        }
    }
}

CacheStats FrameCache::getStats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return pimpl_->stats;
}

void FrameCache::updateConfig(const CacheConfig& config) {
    std::lock_guard<std::mutex> lock(mutex_);
    pimpl_->config = config;
    
    // Si se redujo el tamaño máximo, evictar inmediatamente
    evictIfNeeded();
}

void FrameCache::flushToDisk() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!pimpl_->config.enableDiskCache) {
        return;
    }
    
    for (auto& [frameNum, frame] : pimpl_->frames) {
        if (frame.isKeyframe || pimpl_->importantFrames.count(frameNum)) {
            std::string filename = pimpl_->config.diskCachePath + "/frame_" + 
                                   std::to_string(frameNum) + ".cache";
            
            std::ofstream file(filename, std::ios::binary);
            if (file.is_open()) {
                file.write(reinterpret_cast<const char*>(&frame.frameNumber), sizeof(frame.frameNumber));
                file.write(reinterpret_cast<const char*>(&frame.resolution.width), sizeof(frame.resolution.width));
                file.write(reinterpret_cast<const char*>(&frame.resolution.height), sizeof(frame.resolution.height));
                
                size_t dataSize = frame.data.size();
                file.write(reinterpret_cast<const char*>(&dataSize), sizeof(dataSize));
                file.write(reinterpret_cast<const char*>(frame.data.data()), dataSize);
                
                pimpl_->diskLocations[frameNum] = filename;
                pimpl_->stats.diskWrites++;
            }
        }
    }
}

void FrameCache::prefetchAround(int64_t currentFrame, int range) {
    // Esta función es una sugerencia para el sistema de render
    // La implementación real depende del pipeline de preview
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<int64_t> toReserve;
    for (int i = -range; i <= range; i++) {
        int64_t frameNum = currentFrame + i;
        if (frameNum >= 0 && !pimpl_->frames.count(frameNum)) {
            toReserve.push_back(frameNum);
        }
    }
    
    // Liberar lock antes de llamar a reserve para evitar deadlock
    mutex_.unlock();
    reserve(toReserve);
    mutex_.lock();
}

void FrameCache::evictIfNeeded() {
    // Implementar política de evicción según configuración
    switch (pimpl_->config.evictionPolicy) {
        case CacheEvictionPolicy::LRU:
            evictLRU();
            break;
        case CacheEvictionPolicy::LFU:
            evictLFU();
            break;
        case CacheEvictionPolicy::FIFO:
            evictFIFO();
            break;
        case CacheEvictionPolicy::ADAPTIVE:
            evictAdaptive();
            break;
    }
}

void FrameCache::evictLRU() {
    if (pimpl_->frames.empty()) return;
    
    // Encontrar el frame con el acceso más antiguo
    int64_t oldestFrame = -1;
    auto oldestTime = std::chrono::steady_clock::time_point::max();
    
    for (const auto& [frameNum, accessTime] : pimpl_->accessTimes) {
        // No evictar frames importantes
        if (pimpl_->importantFrames.count(frameNum)) continue;
        
        if (accessTime < oldestTime) {
            oldestTime = accessTime;
            oldestFrame = frameNum;
        }
    }
    
    if (oldestFrame != -1) {
        remove(oldestFrame);
        pimpl_->stats.evictions++;
    }
}

void FrameCache::evictLFU() {
    if (pimpl_->frames.empty()) return;
    
    // Encontrar el frame con menos accesos
    int64_t leastFrequentFrame = -1;
    size_t minCount = SIZE_MAX;
    
    for (const auto& [frameNum, count] : pimpl_->accessCounts) {
        if (pimpl_->importantFrames.count(frameNum)) continue;
        
        if (count < minCount) {
            minCount = count;
            leastFrequentFrame = frameNum;
        }
    }
    
    if (leastFrequentFrame != -1) {
        remove(leastFrequentFrame);
        pimpl_->stats.evictions++;
    }
}

void FrameCache::evictFIFO() {
    if (pimpl_->fifoQueue.empty()) return;
    
    // Evictar el frame que llegó primero
    while (!pimpl_->fifoQueue.empty()) {
        int64_t frameNum = pimpl_->fifoQueue.front();
        pimpl_->fifoQueue.pop();
        
        // Verificar si aún está en cache (podría haber sido removido)
        if (pimpl_->frames.count(frameNum) && !pimpl_->importantFrames.count(frameNum)) {
            remove(frameNum);
            pimpl_->stats.evictions++;
            break;
        }
    }
}

void FrameCache::evictAdaptive() {
    // Implementación simplificada de Adaptive Replacement Cache
    // Combina LRU y LFU dinámicamente
    if (pimpl_->stats.hitRate() > 0.5) {
        // Buena tasa de hits, usar LFU
        evictLFU();
    } else {
        // Mala tasa de hits, usar LRU
        evictLRU();
    }
}

void FrameCache::updateAccessTime(int64_t frameNumber) {
    pimpl_->accessTimes[frameNumber] = std::chrono::steady_clock::now();
}

size_t FrameCache::calculateMemoryUsage(const CachedFrame& frame) const {
    return sizeof(CachedFrame) + frame.data.size();
}

} // namespace ccos::preview
