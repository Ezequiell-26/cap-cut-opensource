#include "media/MediaInterfaces.hpp"

namespace ccos::media {

// Implementaciones placeholder para las interfaces
// Las implementaciones reales estarán en FFmpegAdapter, etc.

class MediaFactory : public IMediaFactory {
public:
    std::unique_ptr<IMediaProbe> createProbe() override {
        // Será implementado por FFmpegProbe
        return nullptr;
    }
    
    std::unique_ptr<IMediaDecoder> createDecoder() override {
        // Será implementado por FFmpegDecoder
        return nullptr;
    }
    
    std::unique_ptr<IMediaImporter> createImporter() override {
        // Será implementado por MediaImporterImpl
        return nullptr;
    }
    
    std::unique_ptr<IMediaCache> createCache(size_t maxSizeBytes) override {
        // Será implementado por LRUMediaCache
        return nullptr;
    }
    
    std::unique_ptr<IProxyGenerator> createProxyGenerator() override {
        // Será implementado por ProxyGeneratorImpl
        return nullptr;
    }
    
    std::unique_ptr<IThumbnailGenerator> createThumbnailGenerator() override {
        // Será implementado por ThumbnailGeneratorImpl
        return nullptr;
    }
    
    std::unique_ptr<IWaveformGenerator> createWaveformGenerator() override {
        // Será implementado por WaveformGeneratorImpl
        return nullptr;
    }
};

std::unique_ptr<IMediaFactory> createMediaFactory() {
    return std::make_unique<MediaFactory>();
}

} // namespace ccos::media
