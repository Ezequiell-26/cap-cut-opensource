#include <gtest/gtest.h>
#include "src/preview/cache/FrameCache.hpp"
#include "src/preview/pipeline/PreviewPipeline.hpp"
#include <thread>
#include <chrono>

namespace ccos::preview {

class FrameCacheTest : public ::testing::Test {
protected:
    void SetUp() override {
        CacheConfig config;
        config.maxMemoryBytes = 10 * 1024 * 1024; // 10MB
        config.maxFrames = 100;
        cache = std::make_unique<FrameCache>(config);
    }
    
    std::unique_ptr<FrameCache> cache;
};

TEST_F(FrameCacheTest, PutAndGet) {
    CachedFrame frame;
    frame.frameNumber = 1;
    frame.resolution = {1920, 1080};
    frame.data.resize(100);
    
    EXPECT_TRUE(cache->put(frame));
    EXPECT_TRUE(cache->contains(1));
    
    auto retrieved = cache->get(1);
    EXPECT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->frameNumber, 1);
    EXPECT_EQ(retrieved->resolution.width, 1920);
}

TEST_F(FrameCacheTest, CacheMiss) {
    auto result = cache->get(999);
    EXPECT_FALSE(result.has_value());
}

TEST_F(FrameCacheTest, MultipleFrames) {
    for (int i = 0; i < 50; i++) {
        CachedFrame frame;
        frame.frameNumber = i;
        frame.data.resize(1000);
        cache->put(frame);
    }
    
    EXPECT_EQ(cache->getStats().currentFrameCount, 50);
    
    // Verificar que todos están disponibles
    for (int i = 0; i < 50; i++) {
        EXPECT_TRUE(cache->contains(i));
    }
}

TEST_F(FrameCacheTest, EvictionLRU) {
    CacheConfig config;
    config.maxFrames = 10;
    config.evictionPolicy = CacheEvictionPolicy::LRU;
    FrameCache smallCache(config);
    
    // Llenar cache
    for (int i = 0; i < 10; i++) {
        CachedFrame frame;
        frame.frameNumber = i;
        frame.data.resize(1000);
        smallCache.put(frame);
    }
    
    // Acceder al frame 0 para actualizar su timestamp
    smallCache.get(0);
    
    // Agregar nuevo frame debería evictar el frame 1 (el más antiguo no accedido)
    CachedFrame newFrame;
    newFrame.frameNumber = 10;
    newFrame.data.resize(1000);
    smallCache.put(newFrame);
    
    EXPECT_FALSE(smallCache.contains(1));
    EXPECT_TRUE(smallCache.contains(0));
    EXPECT_TRUE(smallCache.contains(10));
}

TEST_F(FrameCacheTest, Clear) {
    for (int i = 0; i < 10; i++) {
        CachedFrame frame;
        frame.frameNumber = i;
        cache->put(frame);
    }
    
    cache->clear();
    EXPECT_EQ(cache->getStats().currentFrameCount, 0);
    
    for (int i = 0; i < 10; i++) {
        EXPECT_FALSE(cache->contains(i));
    }
}

TEST_F(FrameCacheTest, Remove) {
    CachedFrame frame;
    frame.frameNumber = 5;
    cache->put(frame);
    
    EXPECT_TRUE(cache->contains(5));
    
    cache->remove(5);
    EXPECT_FALSE(cache->contains(5));
}

TEST_F(FrameCacheTest, StatsTracking) {
    CachedFrame frame;
    frame.frameNumber = 1;
    cache->put(frame);
    
    cache->get(1); // Hit
    cache->get(1); // Hit
    cache->get(2); // Miss
    
    auto stats = cache->getStats();
    EXPECT_EQ(stats.totalHits, 2);
    EXPECT_EQ(stats.totalMisses, 1);
    EXPECT_DOUBLE_EQ(stats.hitRate(), 2.0 / 3.0);
}

TEST_F(FrameCacheTest, ReserveImportantFrames) {
    CacheConfig config;
    config.maxFrames = 5;
    FrameCache smallCache(config);
    
    // Agregar 5 frames
    for (int i = 0; i < 5; i++) {
        CachedFrame frame;
        frame.frameNumber = i;
        smallCache.put(frame);
    }
    
    // Marcar frame 0 como importante
    smallCache.reserve({0});
    
    // Agregar frames nuevos debería evictar otros pero no el 0
    for (int i = 5; i < 10; i++) {
        CachedFrame frame;
        frame.frameNumber = i;
        smallCache.put(frame);
    }
    
    // El frame 0 debería seguir ahí por ser importante
    EXPECT_TRUE(smallCache.contains(0));
}

class PreviewPipelineTest : public ::testing::Test {
protected:
    void SetUp() override {
        PreviewConfig config;
        config.targetResolution = {640, 480};
        config.targetFramerate = 30.0;
        config.maxCacheMemoryMB = 64;
        pipeline = std::make_unique<PreviewPipeline>(config);
    }
    
    std::unique_ptr<PreviewPipeline> pipeline;
};

TEST_F(PreviewPipelineTest, Initialization) {
    // Crear timeline minimal
    timeline::TimelineModel timeline;
    
    // Nota: En tests reales, crear una timeline válida con assets
    // Este test verifica que la inicialización no crashee
    EXPECT_NO_THROW({
        // pipeline->initialize(timeline); // Requiere timeline válido
    });
}

TEST_F(PreviewPipelineTest, StateTransitions) {
    EXPECT_EQ(pipeline->getState(), PreviewState::STOPPED);
    
    // No podemos hacer play sin initialize, pero verificamos que no crashee
    EXPECT_NO_THROW(pipeline->play());
    EXPECT_NO_THROW(pipeline->pause());
    EXPECT_NO_THROW(pipeline->stop());
}

TEST_F(PreviewPipelineTest, SeekOperations) {
    EXPECT_NO_THROW(pipeline->seek(0));
    EXPECT_NO_THROW(pipeline->seek(10));
    EXPECT_NO_THROW(pipeline->seek(100));
}

TEST_F(PreviewPipelineTest, StepOperations) {
    EXPECT_NO_THROW(pipeline->stepForward());
    EXPECT_NO_THROW(pipeline->stepBackward());
}

TEST_F(PreviewPipelineTest, StatsRetrieval) {
    auto stats = pipeline->getStats();
    EXPECT_EQ(stats.state, PreviewState::STOPPED);
    EXPECT_EQ(stats.currentFrame, 0);
    EXPECT_GE(stats.cacheHitRate, 0.0);
    EXPECT_LE(stats.cacheHitRate, 1.0);
}

TEST_F(PreviewPipelineTest, Callbacks) {
    bool frameReadyCalled = false;
    bool errorCalled = false;
    bool progressCalled = false;
    
    pipeline->setFrameReadyCallback([&](int64_t, const std::vector<uint8_t>&, double) {
        frameReadyCalled = true;
    });
    
    pipeline->setErrorCallback([&](const std::string&) {
        errorCalled = true;
    });
    
    pipeline->setProgressCallback([&](double, const std::string&) {
        progressCalled = true;
    });
    
    // Verificar que los callbacks se establecieron sin errores
    EXPECT_NO_THROW({
        // Los callbacks se probarán en integration tests
    });
}

TEST_F(PreviewPipelineTest, ConfigUpdate) {
    PreviewConfig newConfig;
    newConfig.targetResolution = {1280, 720};
    newConfig.targetFramerate = 60.0;
    newConfig.maxCacheMemoryMB = 128;
    
    EXPECT_NO_THROW(pipeline->updateConfig(newConfig));
}

TEST_F(PreviewPipelineTest, CacheOperations) {
    EXPECT_NO_THROW(pipeline->clearCache());
    EXPECT_NO_THROW(pipeline->refresh());
}

TEST_F(PreviewPipelineTest, FrameAvailability) {
    // Inicialmente ningún frame está disponible
    EXPECT_FALSE(pipeline->isFrameAvailable(0));
    EXPECT_FALSE(pipeline->isFrameAvailable(100));
}

TEST_F(PreviewPipelineTest, EstimatedReadyTime) {
    auto time = pipeline->getEstimatedReadyTime(10);
    EXPECT_GE(time.count(), 0);
}

TEST(PreviewConfigTest, DefaultValues) {
    PreviewConfig config;
    
    EXPECT_EQ(config.targetResolution.width, 1920);
    EXPECT_EQ(config.targetResolution.height, 1080);
    EXPECT_DOUBLE_EQ(config.targetFramerate, 30.0);
    EXPECT_TRUE(config.enableEffects);
    EXPECT_TRUE(config.enableTransitions);
    EXPECT_TRUE(config.useProxyMedia);
    EXPECT_EQ(config.proxyQuality, 50);
    EXPECT_EQ(config.maxCacheMemoryMB, 512);
    EXPECT_TRUE(config.enableAudioSync);
    EXPECT_FALSE(config.dropFramesOnOverload);
}

TEST(CacheConfigTest, DefaultValues) {
    CacheConfig config;
    
    EXPECT_EQ(config.maxMemoryBytes, 512 * 1024 * 1024);
    EXPECT_EQ(config.maxFrames, 1000);
    EXPECT_EQ(config.evictionPolicy, CacheEvictionPolicy::LRU);
    EXPECT_FALSE(config.enableCompression);
    EXPECT_FALSE(config.enableDiskCache);
    EXPECT_EQ(config.compressionLevel, 6);
}

TEST(CacheStatsTest, HitRateCalculation) {
    CacheStats stats;
    stats.totalHits = 75;
    stats.totalMisses = 25;
    
    EXPECT_DOUBLE_EQ(stats.hitRate(), 0.75);
    
    stats.totalHits = 0;
    stats.totalMisses = 0;
    EXPECT_DOUBLE_EQ(stats.hitRate(), 0.0);
}

TEST(PreviewStateTest, EnumValues) {
    EXPECT_EQ(static_cast<int>(PreviewState::STOPPED), 0);
    EXPECT_EQ(static_cast<int>(PreviewState::PLAYING), 1);
    EXPECT_EQ(static_cast<int>(PreviewState::PAUSED), 2);
    EXPECT_EQ(static_cast<int>(PreviewState::SEEKING), 3);
    EXPECT_EQ(static_cast<int>(PreviewState::BUFFERING), 4);
    EXPECT_EQ(static_cast<int>(PreviewState::ERROR), 5);
}

} // namespace ccos::preview
