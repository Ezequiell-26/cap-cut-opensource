#pragma once

#include <string>
#include <vector>
#include <optional>
#include <memory>
#include <chrono>
#include <filesystem>
#include <variant>
#include <unordered_map>

namespace ccos::media {

// Tipos de tiempo precisos usando rational_time
struct RationalTime {
    int64_t value;
    int64_t rate;
    
    RationalTime(int64_t v = 0, int64_t r = 30) : value(v), rate(r) {}
    
    double seconds() const { return static_cast<double>(value) / rate; }
    
    bool operator==(const RationalTime& other) const {
        return value * other.rate == other.value * rate;
    }
    
    bool operator<(const RationalTime& other) const {
        return value * other.rate < other.value * rate;
    }
    
    RationalTime operator+(const RationalTime& other) const {
        if (rate == other.rate) {
            return RationalTime(value + other.value, rate);
        }
        int64_t newRate = rate * other.rate;
        int64_t newValue = value * other.rate + other.value * rate;
        return RationalTime(newValue, newRate);
    }
};

// Metadata de audio
struct AudioMetadata {
    int sampleRate = 48000;
    int channels = 2;
    std::string codec;
    int bitDepth = 16;
    std::string layout; // stereo, mono, 5.1, etc.
};

// Metadata de video
struct VideoMetadata {
    int width = 1920;
    int height = 1080;
    RationalTime frameRate;
    std::string codec;
    std::string colorSpace;
    std::string pixelFormat;
    double aspectRatio = 16.0/9.0;
};

// Metadata completa del medio
struct MediaMetadata {
    std::string path;
    std::string filename;
    int64_t fileSize = 0;
    std::string format;
    std::string mimeType;
    
    std::optional<VideoMetadata> video;
    std::optional<AudioMetadata> audio;
    
    RationalTime duration;
    RationalTime startTime;
    
    bool hasVideo() const { return video.has_value(); }
    bool hasAudio() const { return audio.has_value(); }
    
    std::string getPrimaryType() const {
        if (hasVideo() && hasAudio()) return "video";
        if (hasVideo()) return "video";
        if (hasAudio()) return "audio";
        return "unknown";
    }
};

// Resultado del probeo
enum class ProbeStatus {
    SUCCESS,
    FILE_NOT_FOUND,
    INVALID_FORMAT,
    CORRUPTED,
    TIMEOUT,
    PERMISSION_DENIED,
    UNKNOWN_ERROR
};

struct ProbeResult {
    ProbeStatus status;
    std::string errorMessage;
    MediaMetadata metadata;
    std::chrono::milliseconds probeDuration;
    
    bool isSuccess() const { return status == ProbeStatus::SUCCESS; }
};

// Stream de medio
enum class StreamType {
    VIDEO,
    AUDIO,
    SUBTITLE,
    DATA,
    ATTACHMENT
};

struct MediaStream {
    int index;
    StreamType type;
    std::string codec;
    std::string language;
    bool isDefault = false;
    bool isEnabled = true;
    std::unordered_map<std::string, std::string> metadata;
};

// Asset de medio con ID estable
class MediaAsset {
public:
    using Id = std::string;
    
private:
    Id id_;
    std::filesystem::path path_;
    MediaMetadata metadata_;
    std::vector<MediaStream> streams_;
    bool isValid_ = false;
    std::string errorMessage_;
    
public:
    explicit MediaAsset(const Id& id, const std::filesystem::path& path);
    
    const Id& getId() const { return id_; }
    const std::filesystem::path& getPath() const { return path_; }
    const MediaMetadata& getMetadata() const { return metadata_; }
    const std::vector<MediaStream>& getStreams() const { return streams_; }
    
    bool isValid() const { return isValid_; }
    const std::string& getErrorMessage() const { return errorMessage_; }
    
    void setMetadata(const MediaMetadata& metadata);
    void addStream(const MediaStream& stream);
    void invalidate(const std::string& reason);
    
    bool hasVideo() const { return metadata_.hasVideo(); }
    bool hasAudio() const { return metadata_.hasAudio(); }
    
    std::optional<MediaStream> getVideoStream() const;
    std::optional<MediaStream> getAudioStream() const;
};

// Clave de caché
struct MediaCacheKey {
    std::string assetId;
    int streamIndex;
    int64_t startTime;
    int64_t endTime;
    std::string operation; // thumbnail, proxy, waveform, etc.
    std::string parameters; // resolution, format, etc.
    
    bool operator==(const MediaCacheKey& other) const {
        return assetId == other.assetId &&
               streamIndex == other.streamIndex &&
               startTime == other.startTime &&
               endTime == other.endTime &&
               operation == other.operation &&
               parameters == other.parameters;
    }
};

// Hash para MediaCacheKey
struct MediaCacheKeyHash {
    size_t operator()(const MediaCacheKey& key) const {
        size_t h1 = std::hash<std::string>{}(key.assetId);
        size_t h2 = std::hash<int>{}(key.streamIndex);
        size_t h3 = std::hash<int64_t>{}(key.startTime);
        size_t h4 = std::hash<int64_t>{}(key.endTime);
        size_t h5 = std::hash<std::string>{}(key.operation);
        size_t h6 = std::hash<std::string>{}(key.parameters);
        return h1 ^ (h2 << 1) ^ (h3 << 2) ^ (h4 << 3) ^ (h5 << 4) ^ (h6 << 5);
    }
};

// Entrada de caché
template<typename T>
struct CacheEntry {
    T data;
    std::chrono::steady_clock::time_point createdAt;
    std::chrono::steady_clock::time_point lastAccessed;
    size_t sizeBytes;
    int hitCount = 0;
    
    CacheEntry(const T& d, size_t size = 0) 
        : data(d), sizeBytes(size) {
        auto now = std::chrono::steady_clock::now();
        createdAt = now;
        lastAccessed = now;
    }
    
    void touch() {
        lastAccessed = std::chrono::steady_clock::now();
        hitCount++;
    }
};

// Excepciones de medio
class MediaException : public std::runtime_error {
public:
    explicit MediaException(const std::string& msg) : std::runtime_error(msg) {}
};

class MediaNotFoundException : public MediaException {
public:
    explicit MediaNotFoundException(const std::string& path)
        : MediaException("Media not found: " + path) {}
};

class MediaInvalidException : public MediaException {
public:
    explicit MediaInvalidException(const std::string& reason)
        : MediaException("Invalid media: " + reason) {}
};

class MediaCorruptedException : public MediaException {
public:
    explicit MediaCorruptedException(const std::string& path)
        : MediaException("Corrupted media: " + path) {}
};

class MediaTimeoutException : public MediaException {
public:
    explicit MediaTimeoutException(const std::string& operation)
        : MediaException("Timeout during " + operation) {}
};

} // namespace ccos::media
