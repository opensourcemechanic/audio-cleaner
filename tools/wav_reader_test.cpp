#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>
#include <cstring>
#include <cmath>
#include "../include/wav_format.h"

// Test WAV reader against known good data
class WAVReaderTest {
public:
    static void testWAVReading(const std::string& filename) {
        std::cout << "Testing WAV file: " << filename << std::endl;
        
        // Test our WAV reader
        WavReader reader;
        std::vector<int16_t> ourData;
        
        if (!reader.open(filename)) {
            std::cerr << "Failed to open with our WAV reader" << std::endl;
            return;
        }
        
        if (!reader.read(ourData)) {
            std::cerr << "Failed to read with our WAV reader" << std::endl;
            return;
        }
        
        reader.close();
        
        AudioFormat format = reader.getFormat();
        
        std::cout << "Our WAV Reader:" << std::endl;
        std::cout << "  Format: " << format.sampleRate << " Hz, " 
                  << format.numChannels << " channels, " 
                  << format.bitsPerSample << " bits" << std::endl;
        std::cout << "  Samples: " << ourData.size() << std::endl;
        std::cout << "  First 10 samples: ";
        for (int i = 0; i < std::min(10, (int)ourData.size()); i++) {
            std::cout << ourData[i] << " ";
        }
        std::cout << std::endl;
        
        // Check for potential endian issues
        bool hasLargeValues = false;
        bool hasExtremeValues = false;
        
        for (size_t i = 0; i < ourData.size(); i++) {
            if (std::abs(ourData[i]) > 30000) hasLargeValues = true;
            if (ourData[i] == -32768 || ourData[i] == 32767) hasExtremeValues = true;
        }
        
        if (hasLargeValues) {
            std::cout << "  ⚠ Large sample values detected" << std::endl;
        }
        if (hasExtremeValues) {
            std::cout << "  ⚠ Extreme values (clipping) detected" << std::endl;
        }
        
        // Test with byte-swapped data
        std::vector<int16_t> swappedData(ourData.size());
        for (size_t i = 0; i < ourData.size(); i++) {
            uint16_t original = static_cast<uint16_t>(ourData[i]);
            swappedData[i] = static_cast<int16_t>((original >> 8) | (original << 8));
        }
        
        std::cout << "  Byte-swapped first 10 samples: ";
        for (int i = 0; i < std::min(10, (int)swappedData.size()); i++) {
            std::cout << swappedData[i] << " ";
        }
        std::cout << std::endl;
        
        // Calculate RMS for both versions
        double ourRMS = calculateRMS(ourData);
        double swappedRMS = calculateRMS(swappedData);
        
        std::cout << "  Our data RMS: " << ourRMS << std::endl;
        std::cout << "  Swapped RMS: " << swappedRMS << std::endl;
        
        if (swappedRMS < ourRMS && swappedRMS > 1000) {
            std::cout << "  💡 Byte-swapped data looks more reasonable" << std::endl;
        } else {
            std::cout << "  ✓ Original data looks correct" << std::endl;
        }
        
        std::cout << std::endl;
    }
    
private:
    static double calculateRMS(const std::vector<int16_t>& data) {
        double sum = 0.0;
        for (int16_t sample : data) {
            sum += static_cast<double>(sample) * sample;
        }
        return std::sqrt(sum / data.size());
    }
};

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " <wav_file>" << std::endl;
        return 1;
    }
    
    WAVReaderTest::testWAVReading(argv[1]);
    return 0;
}
