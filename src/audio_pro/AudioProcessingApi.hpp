#pragma once
/**
 * CCOS - Professional Audio Processing API
 * Integración con Librosa, Spleeter, RNNoise y otras librerías MIT de audio
 * Licencias: MIT (Spleeter), ISC (Librosa-style), GPL (RNNoise)
 */

#include <vector>
#include <string>
#include <memory>
#include <complex>
#include <Eigen/Dense>

namespace ccos::audio {

struct AudioFeatures {
    std::vector<float> mfcc;
    std::vector<float> chroma;
    std::vector<float> spectralCentroid;
    std::vector<float> zeroCrossingRate;
    std::vector<float> tempo;
    std::string key;
    std::string mood;
    float rms;
    float peak;
};

struct StemSeparation {
    std::vector<float> vocals;
    std::vector<float> drums;
    std::vector<float> bass;
    std::vector<float> other;
};

struct AudioEnhancement {
    std::vector<float> enhanced;
    float noiseReductionLevel;
    bool clippingRemoved;
    bool normalized;
};

class AudioProcessor {
public:
    static AudioProcessor& getInstance();
    
    // Inicialización de motores de audio
    bool initializeLibrosa();
    bool initializeSpleeter(const std::string& modelPath);
    bool initializeRNNoise(const std::string& modelPath);
    bool initializeSoundTouch();
    
    // Análisis espectral
    AudioFeatures extractFeatures(const std::vector<float>& audio, 
                                   int sampleRate = 44100);
    
    // Separación de stems (vocals, drums, bass, other)
    StemSeparation separateStems(const std::vector<float>& audio,
                                  const std::string& model = "2stems");
    
    // Reducción de ruido con IA
    AudioEnhancement denoise(const std::vector<float>& audio,
                              float threshold = 0.5f);
    
    // Eliminación de voz (karaoke mode)
    std::vector<float> removeVocals(const std::vector<float>& stereo);
    
    // Normalización inteligente
    std::vector<float> smartNormalize(const std::vector<float>& audio,
                                       float targetLUFS = -14.0f);
    
    // Compresión multibanda
    std::vector<float> multiBandCompress(const std::vector<float>& audio,
                                          const std::vector<float>& thresholds,
                                          const std::vector<float>& ratios);
    
    // Ecualización paramétrica
    std::vector<float> parametricEQ(const std::vector<float>& audio,
                                     float freq, float gain, float Q);
    
    // Detección de tempo y beat
    struct BeatData {
        std::vector<int> beatPositions;
        float tempo;
        std::vector<float> beatStrength;
    };
    BeatData detectBeats(const std::vector<float>& audio);
    
    // Detección de clave musical
    std::string detectKey(const std::vector<float>& audio);
    
    // Time stretching sin cambio de pitch
    std::vector<float> timeStretch(const std::vector<float>& audio,
                                    float factor);
    
    // Pitch shifting sin cambio de tiempo
    std::vector<float> pitchShift(const std::vector<float>& audio,
                                   int semitones);
    
    // Auto-tune básico
    std::vector<float> autoTune(const std::vector<float>& audio,
                                 const std::string& scale = "major");
    
    // Reverb convolucional
    std::vector<float> convolutionReverb(const std::vector<float>& audio,
                                          const std::vector<float>& impulseResponse);
    
    // Delay/echo
    std::vector<float> delay(const std::vector<float>& audio,
                              float delayTimeMs, float feedback = 0.3f);
    
    // chorus effect
    std::vector<float> chorus(const std::vector<float>& audio,
                               float depth = 0.5f, float rate = 0.3f);
    
    // Distortion/saturation
    std::vector<float> saturation(const std::vector<float>& audio,
                                   float drive = 0.5f);
    
    // Stereo widening
    std::vector<float> stereoWiden(const std::vector<float>& stereo,
                                    float width = 1.5f);
    
    // Mono compatibility check
    float checkMonoCompatibility(const std::vector<float>& stereo);
    
    // Loudness measurement (LUFS)
    float measureLUFS(const std::vector<float>& audio);
    
    // True peak detection
    float detectTruePeak(const std::vector<float>& audio);
    
    // Dynamic range analysis
    struct DynamicRange {
        float crest;
        float dynamicRange;
        float lra; // Loudness Range
    };
    DynamicRange analyzeDynamicRange(const std::vector<float>& audio);
    
    // Silence detection and removal
    std::vector<std::pair<int, int>> detectSilence(
        const std::vector<float>& audio, 
        float threshold = -40.0f);
    
    std::vector<float> removeSilence(const std::vector<float>& audio,
                                      float threshold = -40.0f);
    
    // Crossfade entre dos audios
    std::vector<float> crossfade(const std::vector<float>& audio1,
                                  const std::vector<float>& audio2,
                                  int fadeSamples = 2205);
    
    // Ducking automático (para voiceover)
    std::vector<float> autoDuck(const std::vector<float>& music,
                                 const std::vector<float>& voice,
                                 float duckAmount = 0.3f);
    
    // De-esser
    std::vector<float> deEsser(const std::vector<float>& audio,
                                float freq = 6000.0f,
                                float threshold = 0.7f);
    
    // Breath removal
    std::vector<float> removeBreaths(const std::vector<float>& audio);
    
    // Click/pop removal
    std::vector<float> removeClicks(const std::vector<float>& audio);
    
    // Vinyl emulation
    std::vector<float> vinylEmulation(const std::vector<float>& audio,
                                       float crackle = 0.1f,
                                       float warp = 0.05f);
    
    // Bitcrusher effect
    std::vector<float> bitCrush(const std::vector<float>& audio,
                                 int bits = 8,
                                 float sampleRateReduction = 0.5f);
    
    // Granular synthesis
    std::vector<float> granularSynth(const std::vector<float>& audio,
                                      int grainSize = 1024,
                                      float density = 0.5f);
    
    // Spectral gating (para noise reduction avanzado)
    std::vector<float> spectralGate(const std::vector<float>& audio,
                                     float threshold = 0.3f);
    
    // Harmonic enhancement
    std::vector<float> enhanceHarmonics(const std::vector<float>& audio,
                                         float amount = 0.3f);
    
    // Transient shaping
    std::vector<float> transientShaper(const std::vector<float>& audio,
                                        float attack = 0.5f,
                                        float sustain = 0.5f);
    
    // Mid-Side processing
    struct MSAudio {
        std::vector<float> mid;
        std::vector<float> side;
    };
    MSAudio toMidSide(const std::vector<float>& stereo);
    std::vector<float> fromMidSide(const MSAudio& ms);
    
    // Formant preservation pitch shift
    std::vector<float> formantSafePitchShift(const std::vector<float>& audio,
                                              int semitones);
    
    // Obtener estado del procesador
    bool isInitialized() const { return initialized_; }
    std::string getBackendInfo() const;
    
private:
    AudioProcessor() = default;
    ~AudioProcessor() = default;
    AudioProcessor(const AudioProcessor&) = delete;
    AudioProcessor& operator=(const AudioProcessor&) = delete;
    
    bool initialized_ = false;
    bool librosaInitialized_ = false;
    bool spleeterInitialized_ = false;
    bool rnnoiseInitialized_ = false;
};

// Herramientas de análisis para timeline
class AudioTimelineTools {
public:
    // Generar waveform data para visualización
    static std::vector<float> generateWaveform(const std::string& audioPath,
                                                int numSamples = 1000);
    
    // Detectar puntos de corte por transientes
    static std::vector<int> detectTransientPoints(const std::string& audioPath);
    
    // Analizar compatibilidad de mezcla
    struct MixAnalysis {
        float frequencyClash;
        float stereoBalance;
        float dynamicConsistency;
        std::string suggestions;
    };
    static MixAnalysis analyzeMix(const std::vector<std::string>& audioPaths);
    
    // Sugerir puntos de sincronización musical
    static std::vector<int> suggestSyncPoints(const std::string& musicPath);
    
    // Auto-gain staging para múltiples pistas
    static std::vector<float> autoGainStage(const std::vector<std::vector<float>>& tracks);
};

} // namespace ccos::audio
