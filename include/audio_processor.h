#pragma once
#include <vector>
#include <complex>
#include <cstdint>
#include <memory>

class AudioProcessorBackend;

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
    
    void fft(std::vector<std::complex<float>>& data);
    void ifft(std::vector<std::complex<float>>& data);
    void applyWindow(std::vector<float>& frame);
    void estimateNoiseSpectrum(const std::vector<int16_t>& audio);
    void spectralSubtraction(std::vector<std::complex<float>>& spectrum);
    void adaptiveEchoCancellation(std::vector<int16_t>& input, std::vector<int16_t>& reference);
    void applyHighPassFilter(std::vector<int16_t>& audio, float cutoffFrequency, int sampleRate, int numChannels = 1);
    void applyLowPassFilter(std::vector<int16_t>& audio, float cutoffFrequency, int sampleRate, int numChannels = 1);
    void normalizeAudio(std::vector<int16_t>& audio, float targetLevel = -6.0f);
    
public:
    AudioProcessor(int fftSize = 1024);
    AudioProcessor(int fftSize, std::unique_ptr<AudioProcessorBackend> processingBackend);
    
    void processEchoCancellation(std::vector<int16_t>& audio, const std::vector<int16_t>& reference);
    void processNoiseReduction(std::vector<int16_t>& audio);
    void processFull(std::vector<int16_t>& audio, const std::vector<int16_t>& reference);
    void processLowFrequencyRemoval(std::vector<int16_t>& audio, float cutoffFrequency = 80.0f, int sampleRate = 44100, int numChannels = 1);
    void processHighFrequencyRemoval(std::vector<int16_t>& audio, float cutoffFrequency = 8000.0f, int sampleRate = 44100, int numChannels = 1);
    void processNormalization(std::vector<int16_t>& audio, float targetLevel = -6.0f);
    void processClippingReduction(std::vector<int16_t>& audio, float threshold = 0.95f);
    
    void setFFTSize(int size);
    int getFFTSize() const { return fftSize; }
    
    void setLearningRate(float rate) { learningRate = rate; }
    float getLearningRate() const { return learningRate; }
};
