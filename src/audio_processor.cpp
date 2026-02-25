#include "../include/audio_processor.h"
#include <cmath>
#include <algorithm>
#include <iostream>

const float PI = 3.14159265359f;

AudioProcessor::AudioProcessor(int fftSize) : fftSize(fftSize), learningRate(0.01f) {
    // Validate FFT size (must be power of 2 and between 128 and 65536)
    if (fftSize < 128 || fftSize > 65536 || (fftSize & (fftSize - 1)) != 0) {
        std::cerr << "Warning: Invalid FFT size " << fftSize << ", using 1024" << std::endl;
        this->fftSize = 1024;
    }
    
    hopSize = this->fftSize / 4; // 75% overlap
    
    // Initialize FFT buffer and window
    fftBuffer.resize(this->fftSize);
    window.resize(this->fftSize);
    
    // Create Hann window
    for (int i = 0; i < this->fftSize; i++) {
        window[i] = 0.5f * (1.0f - cosf(2.0f * M_PI * i / (this->fftSize - 1)));
    }
    
    noiseSpectrum.resize(this->fftSize / 2 + 1);
    adaptiveFilter.resize(this->fftSize / 2 + 1);
    
    std::fill(noiseSpectrum.begin(), noiseSpectrum.end(), 0.0f);
    std::fill(adaptiveFilter.begin(), adaptiveFilter.end(), 0.0f);
}

void AudioProcessor::setFFTSize(int size) {
    // Validate FFT size (must be power of 2 and between 128 and 65536)
    if (size < 128 || size > 65536 || (size & (size - 1)) != 0) {
        std::cerr << "Warning: Invalid FFT size " << size << ", keeping current size " << fftSize << std::endl;
        return;
    }
    
    fftSize = size;
    hopSize = fftSize / 4; // 75% overlap
    
    // Reinitialize buffers with new size
    fftBuffer.resize(fftSize);
    window.resize(fftSize);
    noiseSpectrum.resize(fftSize / 2 + 1);
    adaptiveFilter.resize(fftSize / 2 + 1);
    
    // Recreate Hann window
    for (int i = 0; i < fftSize; i++) {
        window[i] = 0.5f * (1.0f - cosf(2.0f * M_PI * i / (fftSize - 1)));
    }
    
    // Reset noise spectrum and adaptive filter
    std::fill(noiseSpectrum.begin(), noiseSpectrum.end(), 0.0f);
    std::fill(adaptiveFilter.begin(), adaptiveFilter.end(), 0.0f);
    
    std::cout << "FFT size changed to " << fftSize << " points" << std::endl;
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
    const int NOISE_FRAMES = 3;  // Reduced from 10 to use fewer frames
    std::vector<float> frame(fftSize, 0.0f);
    std::vector<std::complex<float>> spectrum(fftSize);
    
    std::fill(noiseSpectrum.begin(), noiseSpectrum.end(), 0.0f);
    
    for (int frameNum = 0; frameNum < NOISE_FRAMES && frameNum * fftSize / 2 < static_cast<int>(audio.size()); ++frameNum) {
        for (int i = 0; i < fftSize && frameNum * fftSize / 2 + i < static_cast<int>(audio.size()); ++i) {
            frame[i] = static_cast<float>(audio[frameNum * fftSize / 2 + i]) / 32768.0f;
        }
        
        applyWindow(frame);
        
        for (int i = 0; i < fftSize; ++i) {
            spectrum[i] = std::complex<float>(frame[i], 0);
        }
        
        fft(spectrum);
        
        for (int i = 0; i <= fftSize / 2; ++i) {
            float magnitude = std::abs(spectrum[i]);
            noiseSpectrum[i] += magnitude * magnitude;
        }
    }
    
    for (int i = 0; i <= fftSize / 2; ++i) {
        noiseSpectrum[i] = sqrtf(std::abs(noiseSpectrum[i]) / NOISE_FRAMES);
        // Scale down the noise estimate to be more conservative
        noiseSpectrum[i] *= 0.5f;
    }
}

void AudioProcessor::spectralSubtraction(std::vector<std::complex<float>>& spectrum) {
    const float ALPHA = 0.8f;  // Much more conservative
    const float BETA = 0.1f;   // Higher floor to preserve more signal
    
    for (int i = 0; i <= fftSize / 2; ++i) {
        float magnitude = std::abs(spectrum[i]);
        float phase = std::arg(spectrum[i]);
        
        float subtractedMagnitude = magnitude - ALPHA * std::abs(noiseSpectrum[i]);
        subtractedMagnitude = std::max(subtractedMagnitude, BETA * magnitude);
        
        spectrum[i] = std::polar(subtractedMagnitude, phase);
        if (i > 0 && i < fftSize / 2) {
            spectrum[fftSize - i] = std::polar(subtractedMagnitude, -phase);
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
    std::cout << "🔧 NOISE REDUCTION ONLY:\n";
    std::cout << "   • Spectral Subtraction Algorithm:\n";
    std::cout << "     - Converts audio to frequency domain using " << fftSize << "-point FFT\n";
    std::cout << "     - Uses 75% overlap windowing for smooth transitions\n";
    std::cout << "     - Frequency resolution: " << (44100.0f / fftSize) << " Hz per bin\n";
    std::cout << "     - Time resolution: " << (fftSize / 44100.0f * 1000) << " ms per frame\n";
    std::cout << "     - Estimates noise profile from first audio frames\n";
    std::cout << "     - Subtracts estimated noise from frequency spectrum\n";
    std::cout << "     - Applies Hann window to reduce spectral artifacts\n";
    std::cout << "     - Converts back to time domain with overlap-add\n\n";
    std::cout << "   • Best for: Stationary background noise (hiss, hum, fan noise)\n";
    std::cout << "   • Limitations: Less effective for rapidly changing noise\n";
    std::cout << "\n";
    
    // Estimate noise spectrum from first few frames
    estimateNoiseSpectrum(audio);
    
    // Process audio in overlapping frames
    std::vector<float> frame(fftSize);
    std::vector<std::complex<float>> spectrum(fftSize);
    std::vector<float> output(audio.size(), 0.0f);
    std::vector<float> windowSum(audio.size(), 0.0f);
    
    for (size_t i = 0; i + fftSize <= audio.size(); i += hopSize) {
        // Extract frame
        for (int j = 0; j < fftSize; j++) {
            frame[j] = static_cast<float>(audio[i + j]) / 32768.0f;
        }
        
        // Apply window
        applyWindow(frame);
        
        // Convert to frequency domain
        for (int j = 0; j < fftSize; j++) {
            spectrum[j] = std::complex<float>(frame[j], 0.0f);
        }
        fft(spectrum);
        
        // Apply spectral subtraction
        spectralSubtraction(spectrum);
        
        // Convert back to time domain
        ifft(spectrum);
        
        // Add to output with overlap-add
        for (int j = 0; j < fftSize; j++) {
            if (i + j < output.size()) {
                output[i + j] += spectrum[j].real();
                windowSum[i + j] += window[j];
            }
        }
    }
    
    // Normalize by window sum with soft clipping
    for (size_t i = 0; i < output.size(); i++) {
        if (windowSum[i] > 0.0f) {
            output[i] /= windowSum[i];
            
            // Apply soft clipping to prevent harsh distortion
            float sample = output[i] * 32768.0f;
            if (sample > 30000.0f) {
                sample = 30000.0f + (sample - 30000.0f) * 0.1f; // Soft limiting
            } else if (sample < -30000.0f) {
                sample = -30000.0f + (sample + 30000.0f) * 0.1f; // Soft limiting
            }
            
            audio[i] = static_cast<int16_t>(std::max(-32768.0f, std::min(32767.0f, sample)));
        }
    }
    
    std::cout << "✅ Applied: Noise reduction only\n";
}

void AudioProcessor::processFull(std::vector<int16_t>& audio, const std::vector<int16_t>& reference) {
    if (!reference.empty()) {
        processEchoCancellation(audio, reference);
    }
    processNoiseReduction(audio);
}

void AudioProcessor::processClippingReduction(std::vector<int16_t>& audio, float threshold) {
    std::cout << "🔧 CLIPPING REDUCTION:\n";
    std::cout << "   • Soft clipping algorithm to smooth distorted peaks\n";
    std::cout << "   • Threshold: " << threshold << " (" << (threshold * 100) << "% of full scale)\n";
    std::cout << "   • Applies gentle compression to clipped regions\n";
    std::cout << "   • Preserves audio dynamics while reducing harsh distortion\n\n";
    
    const int16_t clipThreshold = static_cast<int16_t>(threshold * 32767.0f);
    const int16_t hardClip = 32767;
    int clippedSamples = 0;
    int totalSamples = audio.size();
    
    for (size_t i = 0; i < audio.size(); i++) {
        int16_t sample = audio[i];
        int16_t absSample = std::abs(sample);
        
        if (absSample > clipThreshold) {
            clippedSamples++;
            
            // Apply soft clipping using cubic function
            float normalized = static_cast<float>(sample) / 32768.0f;
            
            if (normalized > threshold) {
                // Soft clipping for positive peaks
                float excess = normalized - threshold;
                float softClipped = threshold + excess * (1.0f - excess * excess * 0.5f);
                audio[i] = static_cast<int16_t>(softClipped * 32768.0f);
            } else if (normalized < -threshold) {
                // Soft clipping for negative peaks
                float excess = -normalized - threshold;
                float softClipped = -(threshold + excess * (1.0f - excess * excess * 0.5f));
                audio[i] = static_cast<int16_t>(softClipped * 32768.0f);
            }
        }
    }
    
    float clippingPercentage = (static_cast<float>(clippedSamples) / totalSamples) * 100.0f;
    std::cout << "   • Clipped samples: " << clippedSamples << " (" << clippingPercentage << "%)\n";
    
    if (clippingPercentage > 0.1f) {
        std::cout << "   ⚠ Significant clipping detected - smoothing applied\n";
    } else if (clippingPercentage > 0.01f) {
        std::cout << "   ✓ Moderate clipping detected - smoothing applied\n";
    } else {
        std::cout << "   ✓ Minimal clipping detected - audio is clean\n";
    }
    
    std::cout << "✅ Applied: Clipping reduction\n";
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
