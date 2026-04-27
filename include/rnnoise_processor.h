#pragma once
#include <vector>
#include <cstdint>
#include <string>
#include <memory>

#ifdef HAVE_RNNOISE
#include <rnnoise.h>
#endif

// RNNoise frame size is fixed at 480 samples (10ms at 48kHz)
static constexpr int RNNOISE_FRAME_SIZE = 480;
static constexpr int RNNOISE_SAMPLE_RATE = 48000;

class RNNoiseProcessor {
public:
    // blend: 0.0 = keep original, 1.0 = full denoised (default 1.0)
    // vadThreshold: frames with VAD below this are blended back toward original
    //               to prevent aggressive gating of quiet speech (default 0.0 = disabled)
    explicit RNNoiseProcessor(float blend = 1.0f, float vadThreshold = 0.0f);
    ~RNNoiseProcessor();

    // Returns true if RNNoise library is available (compiled in)
    static bool isAvailable();

    // Process a mono 16-bit PCM buffer at any sample rate.
    // Resamples internally to 48kHz, applies RNNoise, resamples back.
    // Returns false if RNNoise is not available.
    bool process(std::vector<int16_t>& audio, int sampleRate);

    // Process stereo audio with LINKED VAD — both channels use the MAX VAD
    // of the two so voices panned to one side are never gated independently.
    bool processStereo(std::vector<int16_t>& audio, int sampleRate);

    // VAD (voice activity detection) probability from last processed frame (0.0 - 1.0)
    float lastVAD() const { return lastVad_; }

private:
    float blend_        = 1.0f;  // wet/dry mix
    float vadThreshold_ = 0.0f; // below this VAD, fade toward dry
    float lastVad_      = 0.0f;

#ifdef HAVE_RNNOISE
    DenoiseState* state_ = nullptr;

    // Simple linear resampler helpers
    static std::vector<float> resample(const std::vector<float>& input,
                                       int srcRate, int dstRate);
    static std::vector<float> int16ToFloat(const std::vector<int16_t>& in);
    static void floatToInt16(const std::vector<float>& in, std::vector<int16_t>& out);

    float processMonoFloat(std::vector<float>& samples,
                           const std::vector<float>& dry,
                           float vadOverride = -1.0f);
#endif
};
