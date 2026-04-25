#include <iostream>
#include <vector>
#include <cstdint>
#include <cmath>
#include <fstream>
#include <algorithm>

// WAV header structure
struct WAVHeader {
    char riff[4] = {'R', 'I', 'F', 'F'};
    uint32_t fileSize;
    char wave[4] = {'W', 'A', 'V', 'E'};
    char fmt[4] = {'f', 'm', 't', ' '};
    uint32_t fmtSize = 16;
    uint16_t audioFormat = 1;  // PCM
    uint16_t numChannels = 1;  // Mono
    uint32_t sampleRate = 44100;
    uint32_t byteRate = 44100 * 2;  // sampleRate * channels * bitsPerSample/8
    uint16_t blockAlign = 2;  // channels * bitsPerSample/8
    uint16_t bitsPerSample = 16;
    char data[4] = {'d', 'a', 't', 'a'};
    uint32_t dataSize;
};

int main() {
    const int SAMPLE_RATE = 44100;
    const int DURATION = 10;  // 10 seconds
    const int TOTAL_SAMPLES = SAMPLE_RATE * DURATION;
    
    // Create test signal
    std::vector<int16_t> audio(TOTAL_SAMPLES, 0);
    
    // Background noise frequencies (Hz) - for silent sections
    std::vector<float> noiseFreqs = {30.0f, 50.0f, 60.0f};
    
    // Signal frequencies (Hz) - for active sections
    std::vector<float> signalFreqs = {300.0f, 400.0f, 500.0f, 600.0f, 1000.0f, 5000.0f, 10000.0f};
    
    // Generate audio with different sections
    for (int i = 0; i < TOTAL_SAMPLES; i++) {
        float t = static_cast<float>(i) / SAMPLE_RATE;
        float sample = 0.0f;
        
        // Section 0-2s: Pure noise (quiet section for noise estimation)
        if (t < 2.0f) {
            for (float freq : noiseFreqs) {
                sample += 0.1f * sinf(2.0f * M_PI * freq * t);  // Low amplitude noise
            }
        }
        // Section 2-4s: Noise + signals
        else if (t < 4.0f) {
            // Add background noise
            for (float freq : noiseFreqs) {
                sample += 0.1f * sinf(2.0f * M_PI * freq * t);
            }
            // Add test signals
            for (float freq : signalFreqs) {
                sample += 0.3f * sinf(2.0f * M_PI * freq * t);  // Higher amplitude signals
            }
        }
        // Section 4-6s: Pure noise (another quiet section)
        else if (t < 6.0f) {
            for (float freq : noiseFreqs) {
                sample += 0.1f * sinf(2.0f * M_PI * freq * t);
            }
        }
        // Section 6-8s: Noise + signals
        else if (t < 8.0f) {
            for (float freq : noiseFreqs) {
                sample += 0.1f * sinf(2.0f * M_PI * freq * t);
            }
            for (float freq : signalFreqs) {
                sample += 0.3f * sinf(2.0f * M_PI * freq * t);
            }
        }
        // Section 8-10s: Pure noise (final quiet section)
        else {
            for (float freq : noiseFreqs) {
                sample += 0.1f * sinf(2.0f * M_PI * freq * t);
            }
        }
        
        // Convert to int16_t and apply soft clipping to prevent overflow
        float scaled = sample * 16384.0f;  // Scale to 16-bit range
        scaled = std::max(-32768.0f, std::min(32767.0f, scaled));
        audio[i] = static_cast<int16_t>(scaled);
    }
    
    // Create WAV file
    WAVHeader header;
    header.dataSize = TOTAL_SAMPLES * 2;  // samples * 2 bytes per sample
    header.fileSize = sizeof(WAVHeader) + header.dataSize - 8;
    
    std::ofstream outFile("test_frequencies.wav", std::ios::binary);
    if (!outFile) {
        std::cerr << "Error: Cannot create output file" << std::endl;
        return 1;
    }
    
    // Write header
    outFile.write(reinterpret_cast<const char*>(&header), sizeof(WAVHeader));
    
    // Write audio data
    outFile.write(reinterpret_cast<const char*>(audio.data()), header.dataSize);
    outFile.close();
    
    std::cout << "Generated test_frequencies.wav" << std::endl;
    std::cout << "Duration: " << DURATION << " seconds" << std::endl;
    std::cout << "Sample rate: " << SAMPLE_RATE << " Hz" << std::endl;
    std::cout << std::endl;
    std::cout << "Timeline:" << std::endl;
    std::cout << "0-2s:   Background noise only (30, 50, 60 Hz)" << std::endl;
    std::cout << "2-4s:   Noise + signals (300, 400, 500, 600, 1000, 5000, 10000 Hz)" << std::endl;
    std::cout << "4-6s:   Background noise only" << std::endl;
    std::cout << "6-8s:   Noise + signals" << std::endl;
    std::cout << "8-10s:  Background noise only" << std::endl;
    std::cout << std::endl;
    std::cout << "Recommended test commands:" << std::endl;
    std::cout << "1. Process with 50Hz cutoff:" << std::endl;
    std::cout << "   ./audio_cleaner -i test_frequencies.wav -o test_output.wav --noise-reduction --high-pass 50" << std::endl;
    std::cout << std::endl;
    std::cout << "2. Visualize results:" << std::endl;
    std::cout << "   ./audio_viz test_frequencies.wav test_output.wav" << std::endl;
    std::cout << std::endl;
    std::cout << "Expected results:" << std::endl;
    std::cout << "- Noise spectrum should show peaks at 30, 50, 60 Hz" << std::endl;
    std::cout << "- 50Hz high-pass should remove 30Hz noise, reduce 50Hz" << std::endl;
    std::cout << "- Signals at 300Hz+ should be preserved" << std::endl;
    std::cout << "- Difference spectrum should show noise reduction效果" << std::endl;
    
    return 0;
}
