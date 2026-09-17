#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <cmath>
#include <algorithm>
#include <memory>
#include <unordered_map>

namespace ccos::audio {

// ============================================================================
// TIPOS BÁSICOS DE AUDIO
// ============================================================================

using SampleRate = uint32_t;
using ChannelCount = uint8_t;
using FrameCount = uint64_t;

constexpr SampleRate SAMPLE_RATE_44100 = 44100;
constexpr SampleRate SAMPLE_RATE_48000 = 48000;
constexpr SampleRate SAMPLE_RATE_96000 = 96000;

constexpr ChannelCount MONO = 1;
constexpr ChannelCount STEREO = 2;
constexpr ChannelCount SURROUND_51 = 6;
constexpr ChannelCount SURROUND_71 = 8;

// ============================================================================
// FORMATO DE AUDIO
// ============================================================================

enum class AudioFormat : uint8_t {
    FLOAT32,
    INT16,
    INT24,
    INT32
};

inline size_t getBytesPerSample(AudioFormat format) {
    switch (format) {
        case AudioFormat::FLOAT32: return 4;
        case AudioFormat::INT16: return 2;
        case AudioFormat::INT24: return 3;
        case AudioFormat::INT32: return 4;
        default: return 4;
    }
}

// ============================================================================
// BUFFER DE AUDIO
// ============================================================================

struct AudioBuffer {
    std::vector<float> samples; // Interleaved o plano según channels
    SampleRate sampleRate;
    ChannelCount channels;
    FrameCount frameCount;
    
    AudioBuffer() 
        : sampleRate(SAMPLE_RATE_48000)
        , channels(STEREO)
        , frameCount(0) {}
    
    AudioBuffer(FrameCount frames, ChannelCount ch, SampleRate rate = SAMPLE_RATE_48000)
        : sampleRate(rate)
        , channels(ch)
        , frameCount(frames)
        , samples(frames * ch, 0.0f) {}
    
    // Acceder a muestra específica [frame][channel]
    float getSample(FrameCount frame, ChannelCount channel) const {
        if (frame >= frameCount || channel >= channels) return 0.0f;
        return samples[frame * channels + channel];
    }
    
    void setSample(FrameCount frame, ChannelCount channel, float value) {
        if (frame >= frameCount || channel >= channels) return;
        samples[frame * channels + channel] = value;
    }
    
    // Obtener todos los samples de un canal
    std::vector<float> getChannel(ChannelCount channel) const {
        std::vector<float> result(frameCount);
        for (FrameCount i = 0; i < frameCount; ++i) {
            result[i] = getSample(i, channel);
        }
        return result;
    }
    
    // Duración en segundos
    double duration() const {
        return static_cast<double>(frameCount) / sampleRate;
    }
    
    // Tamaño en bytes
    size_t sizeInBytes() const {
        return samples.size() * sizeof(float);
    }
    
    // Clear buffer
    void clear() {
        std::fill(samples.begin(), samples.end(), 0.0f);
    }
    
    // Apply gain
    void applyGain(float gain) {
        if (!std::isfinite(gain)) gain = 1.0f;
        for (auto& sample : samples) {
            if (!std::isfinite(sample)) sample = 0.0f;
            sample = std::clamp(sample * gain, -1.0f, 1.0f);
        }
    }
    
    // Fade in/out
    void fadeIn(FrameCount frames) {
        if (frames == 0 || frameCount == 0) return;
        for (FrameCount i = 0; i < std::min(frames, frameCount); ++i) {
            float factor = static_cast<float>(i) / frames;
            for (ChannelCount ch = 0; ch < channels; ++ch) {
                setSample(i, ch, getSample(i, ch) * factor);
            }
        }
    }
    
    void fadeOut(FrameCount frames) {
        if (frames == 0 || frameCount == 0) return;
        FrameCount start = frameCount > frames ? frameCount - frames : 0;
        for (FrameCount i = start; i < frameCount; ++i) {
            float factor = 1.0f - static_cast<float>(i - start) / frames;
            for (ChannelCount ch = 0; ch < channels; ++ch) {
                setSample(i, ch, getSample(i, ch) * factor);
            }
        }
    }
};

// ============================================================================
// PISTA DE AUDIO
// ============================================================================

enum class TrackType : uint8_t {
    MONO,
    STEREO,
    SURROUND
};

struct AudioTrack {
    std::string id;
    std::string name;
    TrackType type;
    bool muted;
    bool solo;
    float volume; // 0.0 - 1.0
    float pan;    // -1.0 (left) a 1.0 (right)
    
    std::vector<AudioBuffer> clips;
    
    AudioTrack()
        : type(TrackType::STEREO)
        , muted(false)
        , solo(false)
        , volume(1.0f)
        , pan(0.0f) {}
    
    ChannelCount getChannelCount() const {
        switch (type) {
            case TrackType::MONO: return MONO;
            case TrackType::STEREO: return STEREO;
            case TrackType::SURROUND: return SURROUND_51;
            default: return STEREO;
        }
    }
};

// ============================================================================
// BUS DE AUDIO
// ============================================================================

struct AudioBus {
    std::string id;
    std::string name;
    std::vector<std::string> inputTracks;
    std::vector<std::string> outputBuses;
    
    float volume;
    bool muted;
    
    // Efectos en cadena
    std::vector<std::string> effectsChain;
    
    AudioBus()
        : volume(1.0f)
        , muted(false) {}
};

// ============================================================================
// EFECTOS DSP
// ============================================================================

enum class EffectType : uint8_t {
    GAIN,
    PAN,
    EQ,
    COMPRESSOR,
    LIMITER,
    NOISE_GATE,
    REVERB,
    DELAY,
    CHORUS,
    FLANGER,
    DISTORTION,
    HIGH_PASS,
    LOW_PASS,
    NOTCH_FILTER
};

struct EffectParams {
    std::unordered_map<std::string, float> params;
    
    float get(const std::string& name, float defaultValue = 0.0f) const {
        auto it = params.find(name);
        return it != params.end() ? it->second : defaultValue;
    }
    
    void set(const std::string& name, float value) {
        params[name] = value;
    }
};

// ============================================================================
// MEDIDORES DE AUDIO
// ============================================================================

struct AudioMeter {
    float peakLeft;
    float peakRight;
    float rmsLeft;
    float rmsRight;
    float loudnessLUFS;
    float dynamicRange;
    
    AudioMeter()
        : peakLeft(0.0f)
        , peakRight(0.0f)
        , rmsLeft(0.0f)
        , rmsRight(0.0f)
        , loudnessLUFS(-70.0f)
        , dynamicRange(0.0f) {}
};

// ============================================================================
// DISPOSITIVO DE AUDIO
// ============================================================================

struct AudioDevice {
    std::string id;
    std::string name;
    bool isInput;
    bool isOutput;
    SampleRate maxSampleRate;
    ChannelCount maxChannels;
    
    AudioDevice()
        : isInput(false)
        , isOutput(false)
        , maxSampleRate(SAMPLE_RATE_48000)
        , maxChannels(STEREO) {}
};

// ============================================================================
// ESTADO DEL MOTOR DE AUDIO
// ============================================================================

enum class AudioEngineState : uint8_t {
    STOPPED,
    RUNNING,
    PAUSED,
    ERROR
};

// ============================================================================
// CONFIGURACIÓN DEL MOTOR
// ============================================================================

struct AudioEngineConfig {
    SampleRate sampleRate;
    ChannelCount channels;
    size_t bufferSize;
    size_t numBuffers;
    bool enableMonitoring;
    bool enableLatencyCompensation;
    
    AudioEngineConfig()
        : sampleRate(SAMPLE_RATE_48000)
        , channels(STEREO)
        , bufferSize(512)
        , numBuffers(4)
        , enableMonitoring(true)
        , enableLatencyCompensation(true) {}
};

} // namespace ccos::audio
