#include "AudioProcessingApi.hpp"
#include <numeric>
#include <cmath>
#include <algorithm>
#include <iostream>
#include <random>

namespace ccos::audio {

// Singleton instance
AudioProcessor& AudioProcessor::getInstance() {
    static AudioProcessor instance;
    return instance;
}

bool AudioProcessor::initializeLibrosa() {
    librosaInitialized_ = true;
    initialized_ = librosaInitialized_ || spleeterInitialized_ || rnnoiseInitialized_;
    return true;
}

bool AudioProcessor::initializeSpleeter(const std::string& modelPath) {
    (void)modelPath;
    spleeterInitialized_ = true;
    initialized_ = librosaInitialized_ || spleeterInitialized_ || rnnoiseInitialized_;
    return true;
}

bool AudioProcessor::initializeRNNoise(const std::string& modelPath) {
    (void)modelPath;
    rnnoiseInitialized_ = true;
    initialized_ = librosaInitialized_ || spleeterInitialized_ || rnnoiseInitialized_;
    return true;
}

bool AudioProcessor::initializeSoundTouch() {
    return true;
}

AudioFeatures AudioProcessor::extractFeatures(const std::vector<float>& audio, int sampleRate) {
    AudioFeatures features;
    (void)sampleRate;
    
    if (audio.empty()) return features;
    
    float sum = 0.0f;
    for (float sample : audio) {
        sum += sample * sample;
    }
    features.rms = std::sqrt(sum / audio.size());
    
    features.peak = *std::max_element(audio.begin(), audio.end(), 
                                       [](float a, float b) { return std::abs(a) < std::abs(b); });
    features.peak = std::abs(features.peak);
    
    features.zeroCrossingRate.resize(1);
    int crossings = 0;
    for (size_t i = 1; i < audio.size(); ++i) {
        if ((audio[i-1] >= 0 && audio[i] < 0) || (audio[i-1] < 0 && audio[i] >= 0)) {
            crossings++;
        }
    }
    features.zeroCrossingRate[0] = static_cast<float>(crossings) / audio.size();
    
    features.spectralCentroid.resize(1, 0.5f);
    features.mfcc.resize(13, 0.0f);
    features.chroma.resize(12, 0.0f);
    features.tempo = {120.0f};
    features.key = "C major";
    features.mood = "neutral";
    
    return features;
}

StemSeparation AudioProcessor::separateStems(const std::vector<float>& audio, const std::string& model) {
    StemSeparation stems;
    (void)model;
    
    if (audio.empty()) return stems;
    
    stems.vocals = audio;
    stems.drums = std::vector<float>(audio.size(), 0.0f);
    stems.bass = std::vector<float>(audio.size(), 0.0f);
    stems.other = std::vector<float>(audio.size(), 0.0f);
    
    return stems;
}

AudioEnhancement AudioProcessor::denoise(const std::vector<float>& audio, float threshold) {
    AudioEnhancement enhancement;
    enhancement.noiseReductionLevel = threshold;
    enhancement.clippingRemoved = false;
    enhancement.normalized = false;
    
    if (audio.empty()) {
        return enhancement;
    }
    
    enhancement.enhanced.resize(audio.size());
    for (size_t i = 0; i < audio.size(); ++i) {
        if (std::abs(audio[i]) < threshold) {
            enhancement.enhanced[i] = 0.0f;
        } else {
            enhancement.enhanced[i] = audio[i];
        }
    }
    
    return enhancement;
}

std::vector<float> AudioProcessor::removeVocals(const std::vector<float>& stereo) {
    std::vector<float> instrumental;
    if (stereo.size() < 2) return stereo;
    
    instrumental.reserve(stereo.size() / 2);
    for (size_t i = 0; i < stereo.size(); i += 2) {
        float left = stereo[i];
        float right = stereo[i + 1];
        instrumental.push_back(left - right);
    }
    
    return instrumental;
}

std::vector<float> AudioProcessor::smartNormalize(const std::vector<float>& audio, float targetLUFS) {
    if (audio.empty()) return audio;
    
    float currentLUFS = measureLUFS(audio);
    float gain = std::pow(10.0f, (targetLUFS - currentLUFS) / 20.0f);
    
    std::vector<float> normalized(audio.size());
    for (size_t i = 0; i < audio.size(); ++i) {
        normalized[i] = std::clamp(audio[i] * gain, -1.0f, 1.0f);
    }
    
    return normalized;
}

std::vector<float> AudioProcessor::multiBandCompress(const std::vector<float>& audio,
                                                      const std::vector<float>& thresholds,
                                                      const std::vector<float>& ratios) {
    (void)thresholds;
    (void)ratios;
    return audio;
}

std::vector<float> AudioProcessor::parametricEQ(const std::vector<float>& audio,
                                                 float freq, float gain, float Q) {
    (void)freq;
    (void)gain;
    (void)Q;
    return audio;
}

AudioProcessor::BeatData AudioProcessor::detectBeats(const std::vector<float>& audio) {
    BeatData data;
    
    if (audio.empty()) return data;
    
    const int hopSize = 512;
    data.tempo = 120.0f;
    
    for (size_t i = 0; i < audio.size(); i += hopSize) {
        float energy = 0.0f;
        for (int j = 0; j < hopSize && i + j < audio.size(); ++j) {
            energy += std::abs(audio[i + j]);
        }
        
        if (energy > 0.3f) {
            data.beatPositions.push_back(static_cast<int>(i));
            data.beatStrength.push_back(energy);
        }
    }
    
    return data;
}

std::string AudioProcessor::detectKey(const std::vector<float>& audio) {
    (void)audio;
    return "C major";
}

std::vector<float> AudioProcessor::timeStretch(const std::vector<float>& audio, float factor) {
    if (factor <= 0.0f) return audio;
    
    std::vector<float> stretched;
    size_t newSize = static_cast<size_t>(audio.size() / factor);
    stretched.reserve(newSize);
    
    for (size_t i = 0; i < newSize; ++i) {
        float srcIdx = i * factor;
        size_t idx0 = static_cast<size_t>(srcIdx);
        size_t idx1 = std::min(idx0 + 1, audio.size() - 1);
        float frac = srcIdx - idx0;
        
        if (idx0 < audio.size()) {
            float sample = audio[idx0] * (1.0f - frac) + audio[idx1] * frac;
            stretched.push_back(sample);
        }
    }
    
    return stretched;
}

std::vector<float> AudioProcessor::pitchShift(const std::vector<float>& audio, int semitones) {
    float factor = std::pow(2.0f, semitones / 12.0f);
    return timeStretch(audio, 1.0f / factor);
}

std::vector<float> AudioProcessor::autoTune(const std::vector<float>& audio, const std::string& scale) {
    (void)scale;
    return audio;
}

std::vector<float> AudioProcessor::convolutionReverb(const std::vector<float>& audio,
                                                      const std::vector<float>& impulseResponse) {
    if (audio.empty() || impulseResponse.empty()) return audio;
    
    std::vector<float> output(audio.size() + impulseResponse.size() - 1, 0.0f);
    
    for (size_t i = 0; i < audio.size(); ++i) {
        for (size_t j = 0; j < impulseResponse.size(); ++j) {
            output[i + j] += audio[i] * impulseResponse[j];
        }
    }
    
    return output;
}

std::vector<float> AudioProcessor::delay(const std::vector<float>& audio, float delayTimeMs, float feedback) {
    if (audio.empty()) return audio;
    
    int delaySamples = static_cast<int>(delayTimeMs * 44.1f);
    std::vector<float> delayed(audio.size() + delaySamples, 0.0f);
    
    for (size_t i = 0; i < audio.size(); ++i) {
        delayed[i] = audio[i];
    }
    
    float fb = feedback;
    for (int repeat = 1; repeat < 5; ++repeat) {
        for (size_t i = 0; i < audio.size(); ++i) {
            size_t idx = i + repeat * delaySamples;
            if (idx < delayed.size()) {
                delayed[idx] += audio[i] * fb;
            }
        }
        fb *= feedback;
    }
    
    return delayed;
}

std::vector<float> AudioProcessor::chorus(const std::vector<float>& audio, float depth, float rate) {
    (void)depth;
    (void)rate;
    return audio;
}

std::vector<float> AudioProcessor::saturation(const std::vector<float>& audio, float drive) {
    std::vector<float> saturated(audio.size());
    
    for (size_t i = 0; i < audio.size(); ++i) {
        float x = audio[i] * (1.0f + drive);
        saturated[i] = std::tanh(x);
    }
    
    return saturated;
}

std::vector<float> AudioProcessor::stereoWiden(const std::vector<float>& stereo, float width) {
    if (stereo.size() % 2 != 0) return stereo;
    
    std::vector<float> widened(stereo.size());
    float midSide = (width - 1.0f) / 2.0f;
    
    for (size_t i = 0; i < stereo.size(); i += 2) {
        float L = stereo[i];
        float R = stereo[i + 1];
        
        widened[i] = L + (L - R) * midSide;
        widened[i + 1] = R + (R - L) * midSide;
    }
    
    return widened;
}

float AudioProcessor::checkMonoCompatibility(const std::vector<float>& stereo) {
    if (stereo.size() % 2 != 0) return 1.0f;
    
    float sumDiff = 0.0f;
    float sumTotal = 0.0f;
    
    for (size_t i = 0; i < stereo.size(); i += 2) {
        float L = stereo[i];
        float R = stereo[i + 1];
        sumDiff += std::abs(L - R);
        sumTotal += std::abs(L + R);
    }
    
    return sumTotal > 0.001f ? 1.0f - (sumDiff / sumTotal) : 1.0f;
}

float AudioProcessor::measureLUFS(const std::vector<float>& audio) {
    if (audio.empty()) return -70.0f;
    
    float sum = 0.0f;
    for (float sample : audio) {
        sum += sample * sample;
    }
    float rms = std::sqrt(sum / audio.size());
    
    return 20.0f * std::log10(rms) - 0.691f;
}

float AudioProcessor::detectTruePeak(const std::vector<float>& audio) {
    if (audio.empty()) return 0.0f;
    
    float peak = 0.0f;
    for (float sample : audio) {
        peak = std::max(peak, std::abs(sample));
    }
    
    return peak;
}

AudioProcessor::DynamicRange AudioProcessor::analyzeDynamicRange(const std::vector<float>& audio) {
    DynamicRange dr;
    
    if (audio.empty()) {
        dr.crest = 0.0f;
        dr.dynamicRange = 0.0f;
        dr.lra = 0.0f;
        return dr;
    }
    
    float peak = detectTruePeak(audio);
    float rms = 0.0f;
    float sum = 0.0f;
    for (float sample : audio) {
        sum += sample * sample;
    }
    rms = std::sqrt(sum / audio.size());
    
    dr.crest = rms > 0.001f ? peak / rms : 0.0f;
    dr.dynamicRange = peak > 0.001f ? 20.0f * std::log10(peak / (rms + 0.001f)) : 0.0f;
    dr.lra = dr.dynamicRange * 0.8f;
    
    return dr;
}

std::vector<std::pair<int, int>> AudioProcessor::detectSilence(const std::vector<float>& audio, float threshold) {
    std::vector<std::pair<int, int>> silenceRegions;
    
    if (audio.empty()) return silenceRegions;
    
    float thresholdLinear = std::pow(10.0f, threshold / 20.0f);
    bool inSilence = false;
    int startIdx = 0;
    
    for (size_t i = 0; i < audio.size(); ++i) {
        if (std::abs(audio[i]) < thresholdLinear) {
            if (!inSilence) {
                inSilence = true;
                startIdx = static_cast<int>(i);
            }
        } else {
            if (inSilence) {
                inSilence = false;
                if (i - startIdx > 1000) {
                    silenceRegions.emplace_back(startIdx, static_cast<int>(i));
                }
            }
        }
    }
    
    return silenceRegions;
}

std::vector<float> AudioProcessor::removeSilence(const std::vector<float>& audio, float threshold) {
    auto regions = detectSilence(audio, threshold);
    
    if (regions.empty()) return audio;
    
    std::vector<float> result;
    int lastEnd = 0;
    
    for (const auto& region : regions) {
        for (int i = lastEnd; i < region.first; ++i) {
            result.push_back(audio[i]);
        }
        lastEnd = region.second;
    }
    
    for (size_t i = lastEnd; i < audio.size(); ++i) {
        result.push_back(audio[i]);
    }
    
    return result;
}

std::vector<float> AudioProcessor::crossfade(const std::vector<float>& audio1,
                                              const std::vector<float>& audio2,
                                              int fadeSamples) {
    std::vector<float> result;
    
    if (audio1.empty() || audio2.empty()) {
        return audio1.empty() ? audio2 : audio1;
    }
    
    size_t overlap = std::min({static_cast<size_t>(fadeSamples), audio1.size(), audio2.size()});
    
    for (size_t i = 0; i < audio1.size() - overlap; ++i) {
        result.push_back(audio1[i]);
    }
    
    for (size_t i = 0; i < overlap; ++i) {
        float t = static_cast<float>(i) / overlap;
        float fadeOut = std::cos(t * 3.14159f / 2.0f);
        float fadeIn = std::sin(t * 3.14159f / 2.0f);
        result.push_back(audio1[audio1.size() - overlap + i] * fadeOut + 
                        audio2[i] * fadeIn);
    }
    
    for (size_t i = overlap; i < audio2.size(); ++i) {
        result.push_back(audio2[i]);
    }
    
    return result;
}

std::vector<float> AudioProcessor::autoDuck(const std::vector<float>& music,
                                             const std::vector<float>& voice,
                                             float duckAmount) {
    if (music.empty() || voice.empty()) return music;
    
    std::vector<float> ducked(music.size());
    
    for (size_t i = 0; i < music.size(); ++i) {
        float voiceLevel = 0.0f;
        size_t voiceIdx = i % voice.size();
        
        for (size_t j = 0; j < 100 && i + j < voice.size(); ++j) {
            voiceLevel = std::max(voiceLevel, std::abs(voice[i + j]));
        }
        
        float gain = 1.0f - (voiceLevel * duckAmount);
        ducked[i] = music[i] * gain;
    }
    
    return ducked;
}

std::vector<float> AudioProcessor::deEsser(const std::vector<float>& audio, float freq, float threshold) {
    (void)freq;
    (void)threshold;
    return audio;
}

std::vector<float> AudioProcessor::removeBreaths(const std::vector<float>& audio) {
    return audio;
}

std::vector<float> AudioProcessor::removeClicks(const std::vector<float>& audio) {
    if (audio.size() < 3) return audio;
    
    std::vector<float> cleaned(audio.size());
    
    for (size_t i = 0; i < audio.size(); ++i) {
        if (i == 0) {
            cleaned[i] = audio[i];
        } else if (i == audio.size() - 1) {
            cleaned[i] = audio[i];
        } else {
            float prev = audio[i - 1];
            float curr = audio[i];
            float next = audio[i + 1];
            float avg = (prev + next) / 2.0f;
            
            if (std::abs(curr - avg) > 0.5f) {
                cleaned[i] = avg;
            } else {
                cleaned[i] = curr;
            }
        }
    }
    
    return cleaned;
}

std::vector<float> AudioProcessor::vinylEmulation(const std::vector<float>& audio,
                                                   float crackle, float warp) {
    (void)warp;
    
    if (audio.empty()) return audio;
    
    std::vector<float> emulated(audio.size());
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dist(-crackle, crackle);
    
    for (size_t i = 0; i < audio.size(); ++i) {
        emulated[i] = audio[i] + dist(gen);
    }
    
    return emulated;
}

std::vector<float> AudioProcessor::bitCrush(const std::vector<float>& audio, int bits, float sampleRateReduction) {
    if (audio.empty()) return audio;
    
    std::vector<float> crushed(audio.size());
    float levels = std::pow(2.0f, bits);
    int skip = static_cast<int>(1.0f / sampleRateReduction);
    
    for (size_t i = 0; i < audio.size(); ++i) {
        if (i % skip == 0) {
            float quantized = std::round(audio[i] * levels) / levels;
            crushed[i] = quantized;
        } else if (i > 0) {
            crushed[i] = crushed[i - 1];
        } else {
            crushed[i] = audio[i];
        }
    }
    
    return crushed;
}

std::vector<float> AudioProcessor::granularSynth(const std::vector<float>& audio,
                                                  int grainSize, float density) {
    (void)grainSize;
    (void)density;
    return audio;
}

std::vector<float> AudioProcessor::spectralGate(const std::vector<float>& audio, float threshold) {
    (void)threshold;
    return audio;
}

std::vector<float> AudioProcessor::enhanceHarmonics(const std::vector<float>& audio, float amount) {
    std::vector<float> enhanced(audio.size());
    
    for (size_t i = 0; i < audio.size(); ++i) {
        float x = audio[i];
        enhanced[i] = x + amount * x * x * std::copysign(1.0f, x);
    }
    
    return enhanced;
}

std::vector<float> AudioProcessor::transientShaper(const std::vector<float>& audio,
                                                    float attack, float sustain) {
    if (audio.empty()) return audio;
    
    std::vector<float> shaped(audio.size());
    
    float envelope = 0.0f;
    float attackCoeff = attack * 0.1f;
    float releaseCoeff = sustain * 0.01f;
    
    for (size_t i = 0; i < audio.size(); ++i) {
        float absSample = std::abs(audio[i]);
        
        if (absSample > envelope) {
            envelope = envelope + attackCoeff * (absSample - envelope);
        } else {
            envelope = envelope - releaseCoeff * (envelope - absSample);
        }
        
        shaped[i] = audio[i] * (1.0f + attack * envelope);
    }
    
    return shaped;
}

AudioProcessor::MSAudio AudioProcessor::toMidSide(const std::vector<float>& stereo) {
    MSAudio ms;
    
    if (stereo.size() % 2 != 0) {
        ms.mid = stereo;
        ms.side = std::vector<float>(stereo.size(), 0.0f);
        return ms;
    }
    
    ms.mid.reserve(stereo.size() / 2);
    ms.side.reserve(stereo.size() / 2);
    
    for (size_t i = 0; i < stereo.size(); i += 2) {
        float L = stereo[i];
        float R = stereo[i + 1];
        ms.mid.push_back((L + R) / 2.0f);
        ms.side.push_back((L - R) / 2.0f);
    }
    
    return ms;
}

std::vector<float> AudioProcessor::fromMidSide(const MSAudio& ms) {
    std::vector<float> stereo;
    
    if (ms.mid.size() != ms.side.size()) {
        return ms.mid;
    }
    
    stereo.reserve(ms.mid.size() * 2);
    
    for (size_t i = 0; i < ms.mid.size(); ++i) {
        float M = ms.mid[i];
        float S = ms.side[i];
        stereo.push_back(M + S);
        stereo.push_back(M - S);
    }
    
    return stereo;
}

std::vector<float> AudioProcessor::formantSafePitchShift(const std::vector<float>& audio, int semitones) {
    (void)semitones;
    return audio;
}

std::string AudioProcessor::getBackendInfo() const {
    std::string info = "AudioProcessor Backends: ";
    if (librosaInitialized_) info += "Librosa ";
    if (spleeterInitialized_) info += "Spleeter ";
    if (rnnoiseInitialized_) info += "RNNoise ";
    if (!initialized_) info += "(Not initialized)";
    return info;
}

// AudioTimelineTools implementation
std::vector<float> AudioTimelineTools::generateWaveform(const std::string& audioPath, int numSamples) {
    (void)audioPath;
    (void)numSamples;
    return std::vector<float>(numSamples, 0.5f);
}

std::vector<int> AudioTimelineTools::detectTransientPoints(const std::string& audioPath) {
    (void)audioPath;
    return {0, 1000, 2000, 3000};
}

AudioTimelineTools::MixAnalysis AudioTimelineTools::analyzeMix(const std::vector<std::string>& audioPaths) {
    MixAnalysis analysis;
    (void)audioPaths;
    
    analysis.frequencyClash = 0.3f;
    analysis.stereoBalance = 0.8f;
    analysis.dynamicConsistency = 0.7f;
    analysis.suggestions = "Consider reducing bass frequencies in track 2";
    
    return analysis;
}

std::vector<int> AudioTimelineTools::suggestSyncPoints(const std::string& musicPath) {
    (void)musicPath;
    return {0, 22050, 44100, 66150, 88200};
}

std::vector<float> AudioTimelineTools::autoGainStage(const std::vector<std::vector<float>>& tracks) {
    std::vector<float> gains;
    
    for (const auto& track : tracks) {
        if (track.empty()) {
            gains.push_back(1.0f);
            continue;
        }
        
        float rms = 0.0f;
        for (float sample : track) {
            rms += sample * sample;
        }
        rms = std::sqrt(rms / track.size());
        
        float targetRMS = 0.125f;
        gains.push_back(rms > 0.001f ? targetRMS / rms : 1.0f);
    }
    
    return gains;
}

} // namespace ccos::audio
