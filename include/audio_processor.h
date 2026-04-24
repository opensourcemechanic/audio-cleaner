#pragma once
#include <vector>
#include <complex>
#include <cstdint>
#include <memory>
#include <string>

class AudioProcessorBackend;

struct QuietSection {
    float time_ms;
    int frame_index;
    float energy;
    std::vector<int16_t> before_samples;
    std::vector<int16_t> after_samples;
};

struct ProcessingMetadata {
    std::vector<QuietSection> quiet_sections;
    std::vector<float> noise_spectrum;
    int fft_size = 0;
    float alpha = 0.0f;
    float beta = 0.0f;
};

class AudioProcessor {
private:
    int fftSize;
    int hopSize;
    std::vector<std::complex<float>> fftBuffer;
    std::vector<float> window;
    std::vector<float> noiseSpectrum;
    std::vector<std::complex<float>> adaptiveFilter;
    float learningRate;
    std::unique_ptr<AudioProcessorBackend> backend;
    
    ProcessingMetadata lastMetadata;

    void fft(std::vector<std::complex<float>>& data);
    void ifft(std::vector<std::complex<float>>& data);
    void applyWindow(std::vector<float>& frame);
    void estimateNoiseSpectrum(const std::vector<int16_t>& audio, ProcessingMetadata& meta);
    void spectralSubtraction(std::vector<std::complex<float>>& spectrum,
                             std::vector<std::complex<float>>& spectrumBefore);
    void adaptiveEchoCancellation(std::vector<int16_t>& input, std::vector<int16_t>& reference);
    void applyHighPassFilter(std::vector<int16_t>& audio, float cutoffFrequency, int sampleRate);
    void normalizeAudio(std::vector<int16_t>& audio, float targetLevel = -6.0f);
    
public:
    AudioProcessor(int fftSize = 1024);
    AudioProcessor(int fftSize, std::unique_ptr<AudioProcessorBackend> processingBackend);
    
    void processEchoCancellation(std::vector<int16_t>& audio, const std::vector<int16_t>& reference);
    ProcessingMetadata processNoiseReduction(std::vector<int16_t>& audio);
    ProcessingMetadata processFull(std::vector<int16_t>& audio, const std::vector<int16_t>& reference);
    void processLowFrequencyRemoval(std::vector<int16_t>& audio, float cutoffFrequency = 80.0f);
    void processNormalization(std::vector<int16_t>& audio, float targetLevel = -6.0f);
    void processClippingReduction(std::vector<int16_t>& audio, float threshold = 0.95f);
    
    void setFFTSize(int size);
    int getFFTSize() const { return fftSize; }
    
    void setLearningRate(float rate) { learningRate = rate; }
    float getLearningRate() const { return learningRate; }
};
