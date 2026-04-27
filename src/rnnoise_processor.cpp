#include "../include/rnnoise_processor.h"
#include <algorithm>
#include <cstring>
#include <iostream>
#include <cmath>

// ---------------------------------------------------------------------------
// Stub implementation when RNNoise is not compiled in
// ---------------------------------------------------------------------------
#ifndef HAVE_RNNOISE

RNNoiseProcessor::RNNoiseProcessor(float /*blend*/, float /*vadThreshold*/) {}
RNNoiseProcessor::~RNNoiseProcessor() {}

bool RNNoiseProcessor::isAvailable() { return false; }

bool RNNoiseProcessor::process(std::vector<int16_t>&, int) {
    std::cerr << "RNNoise: library not available (build with -DENABLE_RNNOISE=ON)\n";
    return false;
}

bool RNNoiseProcessor::processStereo(std::vector<int16_t>&, int) {
    std::cerr << "RNNoise: library not available (build with -DENABLE_RNNOISE=ON)\n";
    return false;
}

// ---------------------------------------------------------------------------
// Full implementation when RNNoise IS compiled in
// ---------------------------------------------------------------------------
#else

RNNoiseProcessor::RNNoiseProcessor(float blend, float vadThreshold)
    : blend_(std::max(0.0f, std::min(1.0f, blend)))
    , vadThreshold_(std::max(0.0f, std::min(1.0f, vadThreshold)))
{
    state_ = rnnoise_create(nullptr);
}

RNNoiseProcessor::~RNNoiseProcessor() {
    if (state_) {
        rnnoise_destroy(state_);
        state_ = nullptr;
    }
}

bool RNNoiseProcessor::isAvailable() { return true; }

// Simple linear resampler
std::vector<float> RNNoiseProcessor::resample(const std::vector<float>& input,
                                               int srcRate, int dstRate) {
    if (srcRate == dstRate) return input;

    double ratio = static_cast<double>(dstRate) / srcRate;
    size_t outLen = static_cast<size_t>(std::ceil(input.size() * ratio));
    std::vector<float> output(outLen);

    for (size_t i = 0; i < outLen; ++i) {
        double srcIdx = i / ratio;
        size_t lo = static_cast<size_t>(srcIdx);
        size_t hi = std::min(lo + 1, input.size() - 1);
        double frac = srcIdx - lo;
        output[i] = static_cast<float>(input[lo] * (1.0 - frac) + input[hi] * frac);
    }
    return output;
}

std::vector<float> RNNoiseProcessor::int16ToFloat(const std::vector<int16_t>& in) {
    std::vector<float> out(in.size());
    for (size_t i = 0; i < in.size(); ++i)
        out[i] = static_cast<float>(in[i]);
    return out;
}

void RNNoiseProcessor::floatToInt16(const std::vector<float>& in, std::vector<int16_t>& out) {
    out.resize(in.size());
    for (size_t i = 0; i < in.size(); ++i) {
        float clamped = std::max(-32768.0f, std::min(32767.0f, in[i]));
        out[i] = static_cast<int16_t>(clamped);
    }
}

// Process a single float buffer through RNNoise frames.
// dry is kept for wet/dry blending. vadOverride (if >= 0) forces a specific
// VAD value for this call — used for linked stereo gating.
float RNNoiseProcessor::processMonoFloat(std::vector<float>& samples,
                                         const std::vector<float>& dry,
                                         float vadOverride) {
    if (!state_) return 0.0f;

    size_t numFrames = samples.size() / RNNOISE_FRAME_SIZE;
    float frame[RNNOISE_FRAME_SIZE];
    float vad = 0.0f;

    for (size_t f = 0; f < numFrames; ++f) {
        float* wet = samples.data() + f * RNNOISE_FRAME_SIZE;
        const float* orig = dry.data() + f * RNNOISE_FRAME_SIZE;

        std::memcpy(frame, wet, RNNOISE_FRAME_SIZE * sizeof(float));
        vad = rnnoise_process_frame(state_, frame, frame);

        // If a linked VAD is supplied use that; otherwise use this frame's own VAD
        float effectiveVad = (vadOverride >= 0.0f) ? vadOverride : vad;

        // Compute effective blend: if VAD is below threshold, fade back toward dry
        float frameMix = blend_;
        if (vadThreshold_ > 0.0f && effectiveVad < vadThreshold_) {
            float vadRatio = effectiveVad / vadThreshold_; // 0..1
            frameMix = blend_ * vadRatio;
        }

        // Mix denoised (wet) with original (dry)
        for (int s = 0; s < RNNOISE_FRAME_SIZE; ++s)
            wet[s] = frame[s] * frameMix + orig[s] * (1.0f - frameMix);
    }

    // Handle remainder with zero-padding
    size_t remainder = samples.size() % RNNOISE_FRAME_SIZE;
    if (remainder > 0) {
        float* wet = samples.data() + numFrames * RNNOISE_FRAME_SIZE;
        const float* orig = dry.data() + numFrames * RNNOISE_FRAME_SIZE;

        std::memset(frame, 0, sizeof(frame));
        std::memcpy(frame, wet, remainder * sizeof(float));
        vad = rnnoise_process_frame(state_, frame, frame);

        float effectiveVad = (vadOverride >= 0.0f) ? vadOverride : vad;
        float frameMix = blend_;
        if (vadThreshold_ > 0.0f && effectiveVad < vadThreshold_)
            frameMix = blend_ * (effectiveVad / vadThreshold_);

        for (size_t s = 0; s < remainder; ++s)
            wet[s] = frame[s] * frameMix + orig[s] * (1.0f - frameMix);
    }
    return vad;
}

bool RNNoiseProcessor::process(std::vector<int16_t>& audio, int sampleRate) {
    if (!state_ || audio.empty()) return false;

    std::vector<float> orig = int16ToFloat(audio);
    std::vector<float> up   = resample(orig, sampleRate, RNNOISE_SAMPLE_RATE);
    std::vector<float> dryUp = up; // keep dry copy at 48kHz for blending

    lastVad_ = processMonoFloat(up, dryUp, -1.0f);

    std::vector<float> down = resample(up, RNNOISE_SAMPLE_RATE, sampleRate);
    down.resize(audio.size());
    floatToInt16(down, audio);
    return true;
}

bool RNNoiseProcessor::processStereo(std::vector<int16_t>& audio, int sampleRate) {
    if (!state_ || audio.empty()) return false;

    size_t frames = audio.size() / 2;
    std::vector<float> left(frames), right(frames);

    // De-interleave
    for (size_t i = 0; i < frames; ++i) {
        left[i]  = static_cast<float>(audio[i * 2]);
        right[i] = static_cast<float>(audio[i * 2 + 1]);
    }

    // Upsample both channels to 48kHz
    std::vector<float> upL    = resample(left,  sampleRate, RNNOISE_SAMPLE_RATE);
    std::vector<float> upR    = resample(right, sampleRate, RNNOISE_SAMPLE_RATE);
    std::vector<float> dryUpL = upL;
    std::vector<float> dryUpR = upR;

    // --- PASS 1: collect per-frame VAD for both channels without writing output
    // We need the max VAD across channels so we can gate both identically.
    // Use a temporary state to do a dry-run on the right channel.
    DenoiseState* stateR = rnnoise_create(nullptr);

    std::vector<float> vadL, vadR;
    {
        // Collect left VADs (using state_)
        DenoiseState* savedState = state_;
        float tmp[RNNOISE_FRAME_SIZE];
        size_t nFrames = upL.size() / RNNOISE_FRAME_SIZE;
        vadL.resize(nFrames + 1, 0.0f);
        vadR.resize(nFrames + 1, 0.0f);

        // Temporary separate states for VAD scan so we don't advance main states
        DenoiseState* scanL = rnnoise_create(nullptr);
        DenoiseState* scanR = rnnoise_create(nullptr);
        for (size_t f = 0; f < nFrames; ++f) {
            std::memcpy(tmp, upL.data() + f * RNNOISE_FRAME_SIZE, RNNOISE_FRAME_SIZE * sizeof(float));
            vadL[f] = rnnoise_process_frame(scanL, tmp, tmp);
            std::memcpy(tmp, upR.data() + f * RNNOISE_FRAME_SIZE, RNNOISE_FRAME_SIZE * sizeof(float));
            vadR[f] = rnnoise_process_frame(scanR, tmp, tmp);
        }
        rnnoise_destroy(scanL);
        rnnoise_destroy(scanR);
        (void)savedState;
    }

    // --- PASS 2: process with linked (max) VAD per frame
    size_t nFrames = upL.size() / RNNOISE_FRAME_SIZE;
    float frameL[RNNOISE_FRAME_SIZE], frameR[RNNOISE_FRAME_SIZE];

    for (size_t f = 0; f < nFrames; ++f) {
        float linkedVad = std::max(vadL[f], vadR[f]);
        lastVad_ = linkedVad;

        float frameMix = blend_;
        if (vadThreshold_ > 0.0f && linkedVad < vadThreshold_)
            frameMix = blend_ * (linkedVad / vadThreshold_);

        float* wL = upL.data() + f * RNNOISE_FRAME_SIZE;
        float* wR = upR.data() + f * RNNOISE_FRAME_SIZE;
        const float* dL = dryUpL.data() + f * RNNOISE_FRAME_SIZE;
        const float* dR = dryUpR.data() + f * RNNOISE_FRAME_SIZE;

        std::memcpy(frameL, wL, RNNOISE_FRAME_SIZE * sizeof(float));
        std::memcpy(frameR, wR, RNNOISE_FRAME_SIZE * sizeof(float));
        rnnoise_process_frame(state_, frameL, frameL);
        rnnoise_process_frame(stateR,  frameR, frameR);

        for (int s = 0; s < RNNOISE_FRAME_SIZE; ++s) {
            wL[s] = frameL[s] * frameMix + dL[s] * (1.0f - frameMix);
            wR[s] = frameR[s] * frameMix + dR[s] * (1.0f - frameMix);
        }
    }
    rnnoise_destroy(stateR);

    // Downsample back and re-interleave
    std::vector<float> downL = resample(upL, RNNOISE_SAMPLE_RATE, sampleRate);
    std::vector<float> downR = resample(upR, RNNOISE_SAMPLE_RATE, sampleRate);
    downL.resize(frames);
    downR.resize(frames);

    for (size_t i = 0; i < frames; ++i) {
        audio[i * 2]     = static_cast<int16_t>(std::max(-32768.0f, std::min(32767.0f, downL[i])));
        audio[i * 2 + 1] = static_cast<int16_t>(std::max(-32768.0f, std::min(32767.0f, downR[i])));
    }
    return true;
}

#endif // HAVE_RNNOISE
