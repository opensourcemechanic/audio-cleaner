#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>
#include <cstring>

// Simple WAV file checker to detect endian issues
class WAVChecker {
public:
    static void checkWAVFile(const std::string& filename) {
        std::ifstream file(filename, std::ios::binary);
        if (!file) {
            std::cerr << "Cannot open file: " << filename << std::endl;
            return;
        }
        
        // Read header
        char header[44];
        file.read(header, 44);
        
        if (file.gcount() < 44) {
            std::cerr << "File too small for WAV header" << std::endl;
            return;
        }
        
        // Check RIFF and WAVE format
        if (strncmp(header, "RIFF", 4) != 0) {
            std::cerr << "Not a RIFF file" << std::endl;
            return;
        }
        
        if (strncmp(header + 8, "WAVE", 4) != 0) {
            std::cerr << "Not a WAVE file" << std::endl;
            return;
        }
        
        // Extract format information
        int16_t channels = *reinterpret_cast<int16_t*>(header + 22);
        int32_t sampleRate = *reinterpret_cast<int32_t*>(header + 24);
        int16_t bitsPerSample = *reinterpret_cast<int16_t*>(header + 34);
        
        std::cout << "WAV File Analysis:" << std::endl;
        std::cout << "  Channels: " << channels << std::endl;
        std::cout << "  Sample Rate: " << sampleRate << " Hz" << std::endl;
        std::cout << "  Bits per Sample: " << bitsPerSample << std::endl;
        std::cout << "  Byte Order: Little-endian (standard)" << std::endl;
        
        // Read first few samples to check for obvious issues
        file.seekg(44); // Skip to data
        std::vector<int16_t> samples(100);
        file.read(reinterpret_cast<char*>(samples.data()), samples.size() * sizeof(int16_t));
        
        int samplesRead = file.gcount() / sizeof(int16_t);
        std::cout << "  First " << samplesRead << " samples:" << std::endl;
        
        for (int i = 0; i < std::min(10, samplesRead); i++) {
            std::cout << "    Sample " << i << ": " << samples[i] << " (0x" 
                      << std::hex << samples[i] << std::dec << ")" << std::endl;
        }
        
        // Check for potential endian issues
        bool hasLargeValues = false;
        bool hasAlternatingSigns = false;
        int signChanges = 0;
        
        for (int i = 1; i < samplesRead; i++) {
            if (std::abs(samples[i]) > 30000) hasLargeValues = true;
            if ((samples[i] >= 0 && samples[i-1] < 0) || (samples[i] < 0 && samples[i-1] >= 0)) {
                signChanges++;
            }
        }
        
        if (signChanges > samplesRead * 0.4) hasAlternatingSigns = true;
        
        std::cout << "  Analysis:" << std::endl;
        if (hasLargeValues) {
            std::cout << "    ⚠ Large sample values detected (possible clipping or endian issue)" << std::endl;
        }
        if (hasAlternatingSigns) {
            std::cout << "    ⚠ Frequent sign changes (possible high-frequency content or corruption)" << std::endl;
        }
        if (!hasLargeValues && !hasAlternatingSigns) {
            std::cout << "    ✓ Sample values appear normal" << std::endl;
        }
        
        // Check if it might be big-endian
        std::vector<int16_t> swappedSamples(samplesRead);
        for (int i = 0; i < samplesRead; i++) {
            uint16_t original = static_cast<uint16_t>(samples[i]);
            swappedSamples[i] = static_cast<int16_t>((original >> 8) | (original << 8));
        }
        
        // Compare ranges
        int16_t originalMin = samples[0], originalMax = samples[0];
        int16_t swappedMin = swappedSamples[0], swappedMax = swappedSamples[0];
        
        for (int i = 1; i < samplesRead; i++) {
            originalMin = std::min(originalMin, samples[i]);
            originalMax = std::max(originalMax, samples[i]);
            swappedMin = std::min(swappedMin, swappedSamples[i]);
            swappedMax = std::max(swappedMax, swappedSamples[i]);
        }
        
        std::cout << "  Original range: [" << originalMin << ", " << originalMax << "]" << std::endl;
        std::cout << "  Swapped range: [" << swappedMin << ", " << swappedMax << "]" << std::endl;
        
        if (std::abs(swappedMin) < std::abs(originalMin) && std::abs(swappedMax) < std::abs(originalMax)) {
            std::cout << "  💡 File might be big-endian (swapped bytes look more reasonable)" << std::endl;
        } else {
            std::cout << "  ✓ File appears to be correct little-endian" << std::endl;
        }
    }
};

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " <wav_file>" << std::endl;
        return 1;
    }
    
    WAVChecker::checkWAVFile(argv[1]);
    return 0;
}
