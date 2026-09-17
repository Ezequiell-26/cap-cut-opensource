#include <gtest/gtest.h>
#include "media/MediaTypes.hpp"
#include <thread>
#include <chrono>

using namespace ccos::media;

// Tests para RationalTime
TEST(RationalTimeTest, Construction) {
    RationalTime rt(100, 30);
    EXPECT_EQ(rt.value, 100);
    EXPECT_EQ(rt.rate, 30);
}

TEST(RationalTimeTest, Seconds) {
    RationalTime rt(90, 30);
    EXPECT_DOUBLE_EQ(rt.seconds(), 3.0);
    
    RationalTime rt2(45, 30);
    EXPECT_DOUBLE_EQ(rt2.seconds(), 1.5);
}

TEST(RationalTimeTest, Equality) {
    RationalTime rt1(100, 30);
    RationalTime rt2(200, 60); // equivalente
    EXPECT_TRUE(rt1 == rt2);
    
    RationalTime rt3(100, 24);
    EXPECT_FALSE(rt1 == rt3);
}

TEST(RationalTimeTest, Comparison) {
    RationalTime rt1(100, 30);
    RationalTime rt2(200, 30);
    EXPECT_TRUE(rt1 < rt2);
    EXPECT_FALSE(rt2 < rt1);
}

TEST(RationalTimeTest, Addition) {
    RationalTime rt1(100, 30);
    RationalTime rt2(50, 30);
    RationalTime result = rt1 + rt2;
    EXPECT_EQ(result.value, 150);
    EXPECT_EQ(result.rate, 30);
}

TEST(RationalTimeTest, AdditionDifferentRates) {
    RationalTime rt1(1, 2); // 0.5 segundos
    RationalTime rt2(1, 3); // 0.333... segundos
    RationalTime result = rt1 + rt2;
    // 1/2 + 1/3 = 3/6 + 2/6 = 5/6
    EXPECT_EQ(result.value, 5);
    EXPECT_EQ(result.rate, 6);
}

// Tests para MediaMetadata
TEST(MediaMetadataTest, HasVideoAudio) {
    MediaMetadata meta;
    EXPECT_FALSE(meta.hasVideo());
    EXPECT_FALSE(meta.hasAudio());
    
    meta.video = VideoMetadata();
    EXPECT_TRUE(meta.hasVideo());
    EXPECT_FALSE(meta.hasAudio());
    
    meta.audio = AudioMetadata();
    EXPECT_TRUE(meta.hasVideo());
    EXPECT_TRUE(meta.hasAudio());
}

TEST(MediaMetadataTest, GetPrimaryType) {
    MediaMetadata meta;
    EXPECT_EQ(meta.getPrimaryType(), "unknown");
    
    meta.video = VideoMetadata();
    EXPECT_EQ(meta.getPrimaryType(), "video");
    
    meta.audio = AudioMetadata();
    EXPECT_EQ(meta.getPrimaryType(), "video"); // video tiene prioridad
    
    meta.video.reset();
    EXPECT_EQ(meta.getPrimaryType(), "audio");
}

// Tests para VideoMetadata
TEST(VideoMetadataTest, DefaultValues) {
    VideoMetadata vm;
    EXPECT_EQ(vm.width, 1920);
    EXPECT_EQ(vm.height, 1080);
    EXPECT_EQ(vm.aspectRatio, 16.0/9.0);
}

// Tests para AudioMetadata
TEST(AudioMetadataTest, DefaultValues) {
    AudioMetadata am;
    EXPECT_EQ(am.sampleRate, 48000);
    EXPECT_EQ(am.channels, 2);
    EXPECT_EQ(am.bitDepth, 16);
}

// Tests para ProbeResult
TEST(ProbeResultTest, IsSuccess) {
    ProbeResult result;
    result.status = ProbeStatus::SUCCESS;
    EXPECT_TRUE(result.isSuccess());
    
    result.status = ProbeStatus::FILE_NOT_FOUND;
    EXPECT_FALSE(result.isSuccess());
    
    result.status = ProbeStatus::CORRUPTED;
    EXPECT_FALSE(result.isSuccess());
}

// Tests para MediaStream
TEST(MediaStreamTest, DefaultValues) {
    MediaStream stream;
    stream.index = 0;
    stream.type = StreamType::VIDEO;
    EXPECT_EQ(stream.index, 0);
    EXPECT_EQ(stream.type, StreamType::VIDEO);
    EXPECT_FALSE(stream.isDefault);
    EXPECT_TRUE(stream.isEnabled);
}

// Tests para MediaAsset
TEST(MediaAssetTest, Construction) {
    MediaAsset asset("asset-123", "/path/to/video.mp4");
    EXPECT_EQ(asset.getId(), "asset-123");
    EXPECT_EQ(asset.getPath(), "/path/to/video.mp4");
    EXPECT_FALSE(asset.isValid());
}

TEST(MediaAssetTest, SetMetadata) {
    MediaAsset asset("asset-123", "/path/to/video.mp4");
    
    MediaMetadata meta;
    meta.path = "/path/to/video.mp4";
    meta.filename = "video.mp4";
    meta.video = VideoMetadata();
    
    asset.setMetadata(meta);
    
    EXPECT_TRUE(asset.isValid());
    EXPECT_EQ(asset.getMetadata().path, "/path/to/video.mp4");
    EXPECT_TRUE(asset.hasVideo());
    EXPECT_FALSE(asset.hasAudio());
}

TEST(MediaAssetTest, AddStreams) {
    MediaAsset asset("asset-123", "/path/to/video.mp4");
    
    MediaStream videoStream;
    videoStream.index = 0;
    videoStream.type = StreamType::VIDEO;
    videoStream.codec = "h264";
    
    MediaStream audioStream;
    audioStream.index = 1;
    audioStream.type = StreamType::AUDIO;
    audioStream.codec = "aac";
    
    asset.addStream(videoStream);
    asset.addStream(audioStream);
    
    auto vid = asset.getVideoStream();
    auto aud = asset.getAudioStream();
    
    EXPECT_TRUE(vid.has_value());
    EXPECT_TRUE(aud.has_value());
    EXPECT_EQ(vid->codec, "h264");
    EXPECT_EQ(aud->codec, "aac");
}

TEST(MediaAssetTest, Invalidate) {
    MediaAsset asset("asset-123", "/path/to/video.mp4");
    
    MediaMetadata meta;
    asset.setMetadata(meta);
    EXPECT_TRUE(asset.isValid());
    
    asset.invalidate("File corrupted");
    EXPECT_FALSE(asset.isValid());
    EXPECT_EQ(asset.getErrorMessage(), "File corrupted");
}

TEST(MediaAssetTest, GetStreamsWhenEmpty) {
    MediaAsset asset("asset-123", "/path/to/video.mp4");
    
    EXPECT_FALSE(asset.getVideoStream().has_value());
    EXPECT_FALSE(asset.getAudioStream().has_value());
}

// Tests para CacheEntry
TEST(CacheEntryTest, Construction) {
    CacheEntry<std::string> entry("test data", 1024);
    EXPECT_EQ(entry.data, "test data");
    EXPECT_EQ(entry.sizeBytes, 1024);
    EXPECT_EQ(entry.hitCount, 0);
}

TEST(CacheEntryTest, Touch) {
    CacheEntry<std::string> entry("test data");
    auto initialAccess = entry.lastAccessed;
    
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    entry.touch();
    
    EXPECT_GT(entry.lastAccessed, initialAccess);
    EXPECT_EQ(entry.hitCount, 1);
    
    entry.touch();
    EXPECT_EQ(entry.hitCount, 2);
}

// Tests para MediaCacheKey
TEST(MediaCacheKeyTest, Equality) {
    MediaCacheKey key1{"asset-1", 0, 0, 1000, "thumbnail", "320x180"};
    MediaCacheKey key2{"asset-1", 0, 0, 1000, "thumbnail", "320x180"};
    MediaCacheKey key3{"asset-1", 0, 0, 1000, "thumbnail", "640x360"};
    
    EXPECT_TRUE(key1 == key2);
    EXPECT_FALSE(key1 == key3);
}

// Tests para excepciones
TEST(MediaExceptionTest, MediaNotFoundException) {
    try {
        throw MediaNotFoundException("/nonexistent/path.mp4");
    } catch (const MediaNotFoundException& e) {
        std::string msg = e.what();
        EXPECT_NE(msg.find("Media not found"), std::string::npos);
        EXPECT_NE(msg.find("/nonexistent/path.mp4"), std::string::npos);
    }
}

TEST(MediaExceptionTest, MediaInvalidException) {
    try {
        throw MediaInvalidException("Invalid codec");
    } catch (const MediaInvalidException& e) {
        std::string msg = e.what();
        EXPECT_NE(msg.find("Invalid media"), std::string::npos);
        EXPECT_NE(msg.find("Invalid codec"), std::string::npos);
    }
}

TEST(MediaExceptionTest, MediaCorruptedException) {
    try {
        throw MediaCorruptedException("/corrupted/file.mp4");
    } catch (const MediaCorruptedException& e) {
        std::string msg = e.what();
        EXPECT_NE(msg.find("Corrupted media"), std::string::npos);
    }
}

TEST(MediaExceptionTest, MediaTimeoutException) {
    try {
        throw MediaTimeoutException("probe operation");
    } catch (const MediaTimeoutException& e) {
        std::string msg = e.what();
        EXPECT_NE(msg.find("Timeout during"), std::string::npos);
        EXPECT_NE(msg.find("probe operation"), std::string::npos);
    }
}

TEST(MediaExceptionTest, Inheritance) {
    try {
        throw MediaNotFoundException("/test.mp4");
    } catch (const MediaException& e) {
        // Debe poder capturarse como excepción base
        SUCCEED();
    }
    
    try {
        throw MediaInvalidException("test");
    } catch (const std::runtime_error& e) {
        // Debe poder capturarse como runtime_error
        SUCCEED();
    }
}

// Tests para StreamType
TEST(StreamTypeTest, EnumValues) {
    EXPECT_EQ(static_cast<int>(StreamType::VIDEO), 0);
    EXPECT_EQ(static_cast<int>(StreamType::AUDIO), 1);
    EXPECT_EQ(static_cast<int>(StreamType::SUBTITLE), 2);
    EXPECT_EQ(static_cast<int>(StreamType::DATA), 3);
    EXPECT_EQ(static_cast<int>(StreamType::ATTACHMENT), 4);
}

// Tests para ProbeStatus
TEST(ProbeStatusTest, EnumValues) {
    EXPECT_EQ(static_cast<int>(ProbeStatus::SUCCESS), 0);
    EXPECT_EQ(static_cast<int>(ProbeStatus::FILE_NOT_FOUND), 1);
    EXPECT_EQ(static_cast<int>(ProbeStatus::INVALID_FORMAT), 2);
    EXPECT_EQ(static_cast<int>(ProbeStatus::CORRUPTED), 3);
    EXPECT_EQ(static_cast<int>(ProbeStatus::TIMEOUT), 4);
    EXPECT_EQ(static_cast<int>(ProbeStatus::PERMISSION_DENIED), 5);
    EXPECT_EQ(static_cast<int>(ProbeStatus::UNKNOWN_ERROR), 6);
}

// Test de integración básico
TEST(MediaIntegrationTest, FullWorkflow) {
    // Crear asset
    MediaAsset asset("test-asset", "/tmp/test_video.mp4");
    
    // Configurar metadata
    MediaMetadata meta;
    meta.path = "/tmp/test_video.mp4";
    meta.filename = "test_video.mp4";
    meta.fileSize = 1024000;
    meta.format = "mov,mp4,m4a,3gp,3g2,mj2";
    
    VideoMetadata video;
    video.width = 1920;
    video.height = 1080;
    video.frameRate = RationalTime(30, 1);
    video.codec = "h264";
    
    AudioMetadata audio;
    audio.sampleRate = 48000;
    audio.channels = 2;
    audio.codec = "aac";
    
    meta.video = video;
    meta.audio = audio;
    meta.duration = RationalTime(900, 30); // 30 segundos
    
    asset.setMetadata(meta);
    
    // Agregar streams
    MediaStream videoStream;
    videoStream.index = 0;
    videoStream.type = StreamType::VIDEO;
    videoStream.codec = "h264";
    videoStream.isDefault = true;
    
    MediaStream audioStream;
    audioStream.index = 1;
    audioStream.type = StreamType::AUDIO;
    audioStream.codec = "aac";
    audioStream.language = "eng";
    audioStream.isDefault = true;
    
    asset.addStream(videoStream);
    asset.addStream(audioStream);
    
    // Verificar estado final
    EXPECT_TRUE(asset.isValid());
    EXPECT_TRUE(asset.hasVideo());
    EXPECT_TRUE(asset.hasAudio());
    EXPECT_EQ(asset.getMetadata().duration.seconds(), 30.0);
    EXPECT_EQ(asset.getStreams().size(), 2);
    
    auto vidStream = asset.getVideoStream();
    auto audStream = asset.getAudioStream();
    
    ASSERT_TRUE(vidStream.has_value());
    ASSERT_TRUE(audStream.has_value());
    
    EXPECT_TRUE(vidStream->isDefault);
    EXPECT_TRUE(audStream->isDefault);
    EXPECT_EQ(audStream->language, "eng");
}
