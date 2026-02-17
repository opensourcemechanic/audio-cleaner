#pragma once
#include <vector>
#include <complex>
#include <cstdint>

class AudioProcessor {
private:
    static const int FFT_SIZE = 1024;
    static const int OVERLAP_FACTOR = 4;
    
    std::vector<std::complex<float>> fftBuffer;
    std::vector<float> window;
    std::vector<float> noiseSpectrum;
    std::vector<std::complex<float>> adaptiveFilter;
    float learningRate;
    
    void fft(std::vector<std::complex<float>>& data);
    void ifft(std::vector<std::complex<float>>& data);
    void applyWindow(std::vector<float>& frame);
    void estimateNoiseSpectrum(const std::vector<int16_t>& audio);
    void spectralSubtraction(std::vector<std::complex<float>>& spectrum);
    void adaptiveEchoCancellation(std::vector<int16_t>& input, std::vector<int16_t>& reference);
    void applyHighPassFilter(std::vector<int16_t>& audio, float cutoffFrequency, int sampleRate);
    void normalizeAudio(std::vector<int16_t>& audio, float targetLevel = -6.0f);
    
public:
    AudioProcessor();
    
    void processEchoCancellation(std::vector<int16_t>& audio, const std::vector<int16_t>& reference);
    void processNoiseReduction(std::vector<int16_t>& audio);
    void processFull(std::vector<int16_t>& audio, const std::vector<int16_t>& reference);
    void processLowFrequencyRemoval(std::vector<int16_t>& audio, float cutoffFrequency = 80.0f);
    void processNormalization(std::vector<int16_t>& audio, float targetLevel = -6.0f);
    
    void setLearningRate(float rate) { learningRate = rate; }
    float getLearningRate() const { return learningRate; }
};
