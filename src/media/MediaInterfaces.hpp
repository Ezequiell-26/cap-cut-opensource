#pragma once

#include "media/MediaTypes.hpp"
#include <memory>
#include <functional>
#include <future>

namespace ccos::media {

// Interfaces para el motor de medios

// Interfaz para probeo de medios
class IMediaProbe {
public:
    virtual ~IMediaProbe() = default;
    
    virtual ProbeResult probe(const std::filesystem::path& path) = 0;
    virtual ProbeResult probe(const std::filesystem::path& path, 
                              std::chrono::milliseconds timeout) = 0;
    
    virtual bool isAvailable() const = 0;
    virtual std::string getBackendName() const = 0;
};

// Interfaz para decodificación
struct DecodeRequest {
    std::string assetId;
    int streamIndex;
    int64_t startTime;
    int64_t endTime;
    std::string outputFormat;
    std::unordered_map<std::string, std::string> options;
};

struct DecodeResult {
    bool success;
    std::string errorMessage;
    std::filesystem::path outputPath;
    size_t outputSize;
    std::chrono::milliseconds duration;
};

class IMediaDecoder {
public:
    virtual ~IMediaDecoder() = default;
    
    virtual DecodeResult decode(const DecodeRequest& request) = 0;
    virtual std::future<DecodeResult> decodeAsync(const DecodeRequest& request) = 0;
    
    virtual bool cancel(const std::string& assetId) = 0;
    virtual void cancelAll() = 0;
    
    virtual bool isAvailable() const = 0;
};

// Interfaz para importación
struct ImportOptions {
    bool generateProxy = true;
    bool generateThumbnail = true;
    bool generateWaveform = false;
    std::string proxyResolution = "1280x720";
    std::string thumbnailFormat = "jpg";
    int thumbnailCount = 10;
};

struct ImportResult {
    bool success;
    std::string errorMessage;
    MediaAsset asset;
    std::vector<std::filesystem::path> generatedFiles;
};

class IMediaImporter {
public:
    virtual ~IMediaImporter() = default;
    
    virtual ImportResult import(const std::filesystem::path& path,
                                const ImportOptions& options = ImportOptions{}) = 0;
    
    virtual std::future<ImportResult> importAsync(
        const std::filesystem::path& path,
        const ImportOptions& options = ImportOptions{}) = 0;
    
    virtual bool cancel(const std::string& assetId) = 0;
    
    virtual void setProgressCallback(
        std::function<void(const std::string& assetId, double progress)> callback) = 0;
};

// Interfaz para caché
class IMediaCache {
public:
    virtual ~IMediaCache() = default;
    
    template<typename T>
    virtual bool get(const MediaCacheKey& key, T& value) = 0;
    
    template<typename T>
    virtual void put(const MediaCacheKey& key, const T& value, size_t sizeBytes = 0) = 0;
    
    virtual bool contains(const MediaCacheKey& key) const = 0;
    virtual void remove(const MediaCacheKey& key) = 0;
    virtual void clear() = 0;
    
    virtual size_t getSize() const = 0;
    virtual size_t getMaxSize() const = 0;
    virtual void setMaxSize(size_t maxSizeBytes) = 0;
    
    virtual void cleanup() = 0; // Eliminar entradas antiguas según LRU
};

// Interfaz para generación de proxies
struct ProxyOptions {
    std::string resolution = "1280x720";
    std::string codec = "h264";
    int bitrate = 2500000; // 2.5 Mbps
    std::string format = "mp4";
};

class IProxyGenerator {
public:
    virtual ~IProxyGenerator() = default;
    
    virtual std::filesystem::path generateProxy(
        const MediaAsset& asset,
        const ProxyOptions& options = ProxyOptions{}) = 0;
    
    virtual std::future<std::filesystem::path> generateProxyAsync(
        const MediaAsset& asset,
        const ProxyOptions& options = ProxyOptions{}) = 0;
    
    virtual bool cancel(const std::string& assetId) = 0;
    virtual bool isProxyAvailable(const std::string& assetId) const = 0;
    virtual std::filesystem::path getProxyPath(const std::string& assetId) const = 0;
};

// Interfaz para generación de thumbnails
struct ThumbnailOptions {
    int width = 320;
    int height = 180;
    std::string format = "jpg";
    int quality = 85;
    int count = 10;
    bool evenlySpaced = true;
};

class IThumbnailGenerator {
public:
    virtual ~IThumbnailGenerator() = default;
    
    virtual std::vector<std::filesystem::path> generateThumbnails(
        const MediaAsset& asset,
        const ThumbnailOptions& options = ThumbnailOptions{}) = 0;
    
    virtual std::future<std::vector<std::filesystem::path>> generateThumbnailsAsync(
        const MediaAsset& asset,
        const ThumbnailOptions& options = ThumbnailOptions{}) = 0;
    
    virtual std::filesystem::path generateSingleThumbnail(
        const MediaAsset& asset,
        int64_t timestamp,
        const ThumbnailOptions& options = ThumbnailOptions{}) = 0;
    
    virtual bool cancel(const std::string& assetId) = 0;
};

// Interfaz para generación de waveforms
struct WaveformOptions {
    int width = 1000;
    int height = 100;
    std::string format = "png";
    std::string color = "#00FF00";
    int backgroundColor = 0x00000000; // transparente
    float scale = 1.0f;
};

class IWaveformGenerator {
public:
    virtual ~IWaveformGenerator() = default;
    
    virtual std::filesystem::path generateWaveform(
        const MediaAsset& asset,
        const WaveformOptions& options = WaveformOptions{}) = 0;
    
    virtual std::future<std::filesystem::path> generateWaveformAsync(
        const MediaAsset& asset,
        const WaveformOptions& options = WaveformOptions{}) = 0;
    
    virtual bool cancel(const std::string& assetId) = 0;
    virtual bool isWaveformAvailable(const std::string& assetId) const = 0;
    virtual std::filesystem::path getWaveformPath(const std::string& assetId) const = 0;
};

// Fábrica abstracta para crear implementaciones
class IMediaFactory {
public:
    virtual ~IMediaFactory() = default;
    
    virtual std::unique_ptr<IMediaProbe> createProbe() = 0;
    virtual std::unique_ptr<IMediaDecoder> createDecoder() = 0;
    virtual std::unique_ptr<IMediaImporter> createImporter() = 0;
    virtual std::unique_ptr<IMediaCache> createCache(size_t maxSizeBytes) = 0;
    virtual std::unique_ptr<IProxyGenerator> createProxyGenerator() = 0;
    virtual std::unique_ptr<IThumbnailGenerator> createThumbnailGenerator() = 0;
    virtual std::unique_ptr<IWaveformGenerator> createWaveformGenerator() = 0;
};

} // namespace ccos::media
