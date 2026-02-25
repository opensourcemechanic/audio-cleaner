#include <iostream>
#include <vector>
#include <cstdint>
#include <memory>
#include "../include/audio_loader.h"

// Simple bypass converter to test encoding without processing
class BypassConverter {
public:
    static void convertFile(const std::string& inputFile, const std::string& outputFile) {
        std::cout << "Bypass conversion: " << inputFile << " -> " << outputFile << std::endl;
        
        // Initialize formats
        AudioLoader::registerFormat(std::make_unique<WavFormat>());
        AudioLoader::registerFormat(std::make_unique<Mp3Format>());
        AudioLoader::registerFormat(std::make_unique<OggFormat>());
        AudioLoader::registerFormat(std::make_unique<WmaFormat>());
        
        AudioLoader loader;
        std::vector<int16_t> audioData;
        AudioFormat format;
        
        if (!loader.loadAudio(inputFile, audioData, format)) {
            std::cerr << "Failed to load input file" << std::endl;
            return;
        }
        
        std::cout << "Loaded: " << audioData.size() << " samples, " 
                  << format.sampleRate << " Hz, " 
                  << format.numChannels << " channels" << std::endl;
        
        // Check first few samples
        std::cout << "First 10 samples: ";
        for (int i = 0; i < std::min(10, (int)audioData.size()); i++) {
            std::cout << audioData[i] << " ";
        }
        std::cout << std::endl;
        
        // Save without any processing
        if (!loader.saveAudio(outputFile, audioData, format)) {
            std::cerr << "Failed to save output file" << std::endl;
            return;
        }
        
        std::cout << "✅ Bypass conversion completed" << std::endl;
    }
};

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cout << "Usage: " << argv[0] << " <input_file> <output_file>" << std::endl;
        return 1;
    }
    
    BypassConverter::convertFile(argv[1], argv[2]);
    return 0;
}
