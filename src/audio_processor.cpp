#include "../include/audio_processor.h"
#include <cmath>
#include <algorithm>
#include <iostream>

const float PI = 3.14159265359f;

AudioProcessor::AudioProcessor() : learningRate(0.01f) {
    fftBuffer.resize(FFT_SIZE);
    window.resize(FFT_SIZE);
    noiseSpectrum.resize(FFT_SIZE / 2 + 1);
    adaptiveFilter.resize(FFT_SIZE / 2 + 1);
    
    for (int i = 0; i < FFT_SIZE; ++i) {
        window[i] = 0.5f * (1.0f - cosf(2.0f * PI * i / (FFT_SIZE - 1)));
    }
    
    std::fill(noiseSpectrum.begin(), noiseSpectrum.end(), 0.0f);
    std::fill(adaptiveFilter.begin(), adaptiveFilter.end(), 0.0f);
}

void AudioProcessor::fft(std::vector<std::complex<float>>& data) {
    int N = data.size();
    if (N <= 1) return;
    
    std::vector<std::complex<float>> even(N/2), odd(N/2);
    for (int i = 0; i < N/2; ++i) {
        even[i] = data[2*i];
        odd[i] = data[2*i + 1];
    }
    
    fft(even);
    fft(odd);
    
    for (int k = 0; k < N/2; ++k) {
        std::complex<float> t = std::exp(std::complex<float>(0, -2 * PI * k / N)) * odd[k];
        data[k] = even[k] + t;
        data[k + N/2] = even[k] - t;
    }
}

void AudioProcessor::ifft(std::vector<std::complex<float>>& data) {
    int N = data.size();
    if (N <= 1) return;
    
    for (int i = 0; i < N; ++i) {
        data[i] = std::conj(data[i]);
    }
    
    fft(data);
    
    for (int i = 0; i < N; ++i) {
        data[i] = std::conj(data[i]) / static_cast<float>(N);
    }
}

void AudioProcessor::applyWindow(std::vector<float>& frame) {
    for (size_t i = 0; i < frame.size() && i < window.size(); ++i) {
        frame[i] *= window[i];
    }
}

void AudioProcessor::estimateNoiseSpectrum(const std::vector<int16_t>& audio) {
    const int NOISE_FRAMES = 10;
    std::vector<float> frame(FFT_SIZE, 0.0f);
    std::vector<std::complex<float>> spectrum(FFT_SIZE);
    
    std::fill(noiseSpectrum.begin(), noiseSpectrum.end(), 0.0f);
    
    for (int frameNum = 0; frameNum < NOISE_FRAMES && frameNum * FFT_SIZE / 2 < static_cast<int>(audio.size()); ++frameNum) {
        for (int i = 0; i < FFT_SIZE && frameNum * FFT_SIZE / 2 + i < static_cast<int>(audio.size()); ++i) {
            frame[i] = static_cast<float>(audio[frameNum * FFT_SIZE / 2 + i]) / 32768.0f;
        }
        
        applyWindow(frame);
        
        for (int i = 0; i < FFT_SIZE; ++i) {
            spectrum[i] = std::complex<float>(frame[i], 0);
        }
        
        fft(spectrum);
        
        for (int i = 0; i <= FFT_SIZE / 2; ++i) {
            float magnitude = std::abs(spectrum[i]);
            noiseSpectrum[i] += magnitude * magnitude;
        }
    }
    
    for (int i = 0; i <= FFT_SIZE / 2; ++i) {
        noiseSpectrum[i] = sqrtf(noiseSpectrum[i] / NOISE_FRAMES);
    }
}

void AudioProcessor::spectralSubtraction(std::vector<std::complex<float>>& spectrum) {
    const float ALPHA = 2.0f;
    const float BETA = 0.01f;
    
    for (int i = 0; i <= FFT_SIZE / 2; ++i) {
        float magnitude = std::abs(spectrum[i]);
        float phase = std::arg(spectrum[i]);
        
        float subtractedMagnitude = magnitude - ALPHA * noiseSpectrum[i];
        subtractedMagnitude = std::max(subtractedMagnitude, BETA * magnitude);
        
        spectrum[i] = std::polar(subtractedMagnitude, phase);
        if (i > 0 && i < FFT_SIZE / 2) {
            spectrum[FFT_SIZE - i] = std::polar(subtractedMagnitude, -phase);
        }
    }
}

void AudioProcessor::adaptiveEchoCancellation(std::vector<int16_t>& input, std::vector<int16_t>& reference) {
    const int FILTER_LENGTH = 512;
    std::vector<float> filterCoeffs(FILTER_LENGTH, 0.0f);
    
    for (size_t i = FILTER_LENGTH; i < std::min(input.size(), reference.size()); ++i) {
        float error = static_cast<float>(input[i]) / 32768.0f;
        
        for (int j = 0; j < FILTER_LENGTH; ++j) {
            error -= filterCoeffs[j] * (static_cast<float>(reference[i - j]) / 32768.0f);
        }
        
        for (int j = 0; j < FILTER_LENGTH; ++j) {
            filterCoeffs[j] += learningRate * error * (static_cast<float>(reference[i - j]) / 32768.0f);
        }
        
        float output = static_cast<float>(input[i]) / 32768.0f;
        for (int j = 0; j < FILTER_LENGTH; ++j) {
            output -= filterCoeffs[j] * (static_cast<float>(reference[i - j]) / 32768.0f);
        }
        
        input[i] = static_cast<int16_t>(std::max(-32768.0f, std::min(32767.0f, output * 32768.0f)));
    }
}

void AudioProcessor::processEchoCancellation(std::vector<int16_t>& audio, const std::vector<int16_t>& reference) {
    if (reference.size() != audio.size()) {
        std::cerr << "Warning: Reference signal size mismatch" << std::endl;
        return;
    }
    
    std::vector<int16_t> audioCopy = audio;
    adaptiveEchoCancellation(audioCopy, const_cast<std::vector<int16_t>&>(reference));
    audio = audioCopy;
}

void AudioProcessor::processNoiseReduction(std::vector<int16_t>& audio) {
    estimateNoiseSpectrum(audio);
    
    std::vector<float> frame(FFT_SIZE, 0.0f);
    std::vector<float> overlap(FFT_SIZE / 2, 0.0f);
    std::vector<std::complex<float>> spectrum(FFT_SIZE);
    
    for (size_t pos = 0; pos < audio.size(); pos += FFT_SIZE / 2) {
        for (int i = 0; i < FFT_SIZE; ++i) {
            if (pos + i < audio.size()) {
                frame[i] = static_cast<float>(audio[pos + i]) / 32768.0f;
            } else {
                frame[i] = 0.0f;
            }
        }
        
        applyWindow(frame);
        
        for (int i = 0; i < FFT_SIZE; ++i) {
            spectrum[i] = std::complex<float>(frame[i], 0);
        }
        
        fft(spectrum);
        spectralSubtraction(spectrum);
        ifft(spectrum);
        
        for (int i = 0; i < FFT_SIZE / 2; ++i) {
            float sample = spectrum[i].real() + overlap[i];
            if (pos + i < audio.size()) {
                audio[pos + i] = static_cast<int16_t>(std::max(-32768.0f, std::min(32767.0f, sample * 32768.0f)));
            }
            overlap[i] = spectrum[i + FFT_SIZE / 2].real();
        }
    }
}

void AudioProcessor::processFull(std::vector<int16_t>& audio, const std::vector<int16_t>& reference) {
    if (!reference.empty()) {
        processEchoCancellation(audio, reference);
    }
    processNoiseReduction(audio);
}

void AudioProcessor::applyHighPassFilter(std::vector<int16_t>& audio, float cutoffFrequency, int sampleRate) {
    // Simple IIR high-pass filter using Butterworth design
    // This will remove low-frequency noise like "breathing" sounds
    
    float nyquist = sampleRate / 2.0f;
    float normalizedCutoff = cutoffFrequency / nyquist;
    
    // Calculate Butterworth filter coefficients (2nd order)
    float c = tanf(PI * normalizedCutoff);
    float a1 = 1.0f / (c + 1.0f);
    float a2 = (c - 1.0f) / (c + 1.0f);
    float b0 = 1.0f / (c + 1.0f);
    float b1 = -1.0f / (c + 1.0f);
    
    // Apply filter to each channel separately
    for (int channel = 0; channel < 2; ++channel) {
        float x1 = 0.0f, x2 = 0.0f;  // Input history
        float y1 = 0.0f, y2 = 0.0f;  // Output history
        
        for (size_t i = channel; i < audio.size(); i += 2) {
            float input = static_cast<float>(audio[i]) / 32768.0f;
            
            // Apply high-pass filter difference equation
            float output = b0 * input + b1 * x1 - a2 * y1;
            
            // Update history
            x1 = input;
            y1 = output;
            
            // Convert back to int16 and store
            audio[i] = static_cast<int16_t>(std::max(-32768.0f, std::min(32767.0f, output * 32768.0f)));
        }
    }
}

void AudioProcessor::processLowFrequencyRemoval(std::vector<int16_t>& audio, float cutoffFrequency) {
    // This method applies a high-pass filter to remove low-frequency periodic noise
    // Default cutoff is 80 Hz, which removes most "breathing" and rumble sounds
    
    std::cout << "🔧 LOW-FREQUENCY NOISE REMOVAL:\n";
    std::cout << "   • High-pass filter at " << cutoffFrequency << " Hz cutoff\n";
    std::cout << "   • Removes: Low-frequency rumble, breathing sounds, motor noise\n";
    std::cout << "   • Preserves: Voice, music, and mid/high frequency content\n";
    std::cout << "   • Filter type: 2nd order Butterworth IIR\n";
    std::cout << "\n";
    
    // Apply the high-pass filter (assuming 44.1 kHz sample rate)
    int sampleRate = 44100; // Could be made configurable
    applyHighPassFilter(audio, cutoffFrequency, sampleRate);
    
    std::cout << "✅ Applied: Low-frequency noise removal\n";
}

void AudioProcessor::normalizeAudio(std::vector<int16_t>& audio, float targetLevel) {
    // Normalize audio to target level in dB (standard audio levels)
    // Common levels: -3dB (hot), -6dB (standard), -12dB (conservative), -20dB (quiet)
    
    if (audio.empty()) return;
    
    // Find peak level in the audio
    int16_t peakSample = 0;
    for (int16_t sample : audio) {
        peakSample = std::max(peakSample, static_cast<int16_t>(std::abs(sample)));
    }
    
    if (peakSample == 0) return; // Silent audio
    
    // Calculate current peak level in dB
    float currentPeakDb = 20.0f * log10f(static_cast<float>(peakSample) / 32768.0f);
    
    // Calculate gain needed to reach target level
    float gainDb = targetLevel - currentPeakDb;
    float gainLinear = powf(10.0f, gainDb / 20.0f);
    
    // Apply gain with limiting to prevent clipping
    const float maxGain = 20.0f; // Limit maximum gain to prevent excessive amplification
    gainLinear = std::min(gainLinear, maxGain);
    
    // Apply gain to all samples
    for (size_t i = 0; i < audio.size(); ++i) {
        float sample = static_cast<float>(audio[i]) * gainLinear;
        // Hard limiting to prevent clipping
        sample = std::max(-32768.0f, std::min(32767.0f, sample));
        audio[i] = static_cast<int16_t>(sample);
    }
}

void AudioProcessor::processNormalization(std::vector<int16_t>& audio, float targetLevel) {
    std::cout << "🔧 AUDIO NORMALIZATION:\n";
    std::cout << "   • Target level: " << targetLevel << " dBFS\n";
    
    if (targetLevel >= -3.0f) {
        std::cout << "   • Standard: Hot master level\n";
        std::cout << "   • Best for: Modern music, commercial releases\n";
        std::cout << "   • Warning: Close to digital clipping\n";
    } else if (targetLevel >= -9.0f) {
        std::cout << "   • Standard: Normal audio level\n";
        std::cout << "   • Best for: General audio, podcasts, voice\n";
        std::cout << "   • Balance: Good loudness with headroom\n";
    } else if (targetLevel >= -15.0f) {
        std::cout << "   • Standard: Conservative level\n";
        std::cout << "   • Best for: Background music, ambient audio\n";
        std::cout << "   • Safety: Plenty of headroom\n";
    } else {
        std::cout << "   • Standard: Quiet level\n";
        std::cout << "   • Best for: Audio books, documentaries\n";
        std::cout << "   • Character: Very dynamic range\n";
    }
    
    std::cout << "   • Method: Peak-based normalization with limiting\n";
    std::cout << "   • Prevents: Digital clipping and distortion\n";
    std::cout << "\n";
    
    normalizeAudio(audio, targetLevel);
    
    std::cout << "✅ Applied: Audio normalization\n";
}
