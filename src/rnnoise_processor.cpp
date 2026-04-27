#include "../include/rnnoise_processor.h"
#include <algorithm>
#include <cstring>
#include <iostream>
#include <cmath>

// ---------------------------------------------------------------------------
// Stub implementation when RNNoise is not compiled in
// ---------------------------------------------------------------------------
#ifndef HAVE_RNNOISE

RNNoiseProcessor::RNNoiseProcessor() {}
RNNoiseProcessor::~RNNoiseProcessor() {}

bool RNNoiseProcessor::isAvailable() { return false; }

bool RNNoiseProcessor::process(std::vector<int16_t>&, int) {
    std::cerr << "RNNoise: library not available (build with -DHAVE_RNNOISE)\n";
    return false;
}

bool RNNoiseProcessor::processStereo(std::vector<int16_t>&, int) {
    std::cerr << "RNNoise: library not available (build with -DHAVE_RNNOISE)\n";
    return false;
}

// ---------------------------------------------------------------------------
// Full implementation when RNNoise IS compiled in
// ---------------------------------------------------------------------------
#else

RNNoiseProcessor::RNNoiseProcessor() {
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

bool RNNoiseProcessor::processMonoFloat(std::vector<float>& samples) {
    if (!state_) return false;

    // RNNoise processes fixed RNNOISE_FRAME_SIZE frames
    size_t numFrames = samples.size() / RNNOISE_FRAME_SIZE;
    float frame[RNNOISE_FRAME_SIZE];

    for (size_t f = 0; f < numFrames; ++f) {
        float* src = samples.data() + f * RNNOISE_FRAME_SIZE;
        std::memcpy(frame, src, RNNOISE_FRAME_SIZE * sizeof(float));
        lastVad_ = rnnoise_process_frame(state_, frame, frame);
        std::memcpy(src, frame, RNNOISE_FRAME_SIZE * sizeof(float));
    }

    // Handle any remaining samples (< RNNOISE_FRAME_SIZE) with zero-padding
    size_t remainder = samples.size() % RNNOISE_FRAME_SIZE;
    if (remainder > 0) {
        std::memset(frame, 0, sizeof(frame));
        std::memcpy(frame, samples.data() + numFrames * RNNOISE_FRAME_SIZE,
                    remainder * sizeof(float));
        lastVad_ = rnnoise_process_frame(state_, frame, frame);
        std::memcpy(samples.data() + numFrames * RNNOISE_FRAME_SIZE, frame,
                    remainder * sizeof(float));
    }
    return true;
}

bool RNNoiseProcessor::process(std::vector<int16_t>& audio, int sampleRate) {
    if (!state_ || audio.empty()) return false;

    // Convert to float, resample to 48kHz, denoise, resample back
    std::vector<float> f = int16ToFloat(audio);
    std::vector<float> up = resample(f, sampleRate, RNNOISE_SAMPLE_RATE);

    if (!processMonoFloat(up)) return false;

    std::vector<float> down = resample(up, RNNOISE_SAMPLE_RATE, sampleRate);
    down.resize(audio.size()); // ensure same length
    floatToInt16(down, audio);
    return true;
}

bool RNNoiseProcessor::processStereo(std::vector<int16_t>& audio, int sampleRate) {
    if (!state_ || audio.empty()) return false;

    size_t frames = audio.size() / 2;
    std::vector<int16_t> left(frames), right(frames);

    // De-interleave
    for (size_t i = 0; i < frames; ++i) {
        left[i]  = audio[i * 2];
        right[i] = audio[i * 2 + 1];
    }

    // Process each channel (create a second state for right channel)
    DenoiseState* rightState = rnnoise_create(nullptr);

    process(left, sampleRate);

    // Temporarily swap state for right channel
    DenoiseState* tmp = state_;
    state_ = rightState;
    process(right, sampleRate);
    state_ = tmp;
    rnnoise_destroy(rightState);

    // Re-interleave
    for (size_t i = 0; i < frames; ++i) {
        audio[i * 2]     = left[i];
        audio[i * 2 + 1] = right[i];
    }
    return true;
}

#endif // HAVE_RNNOISE
