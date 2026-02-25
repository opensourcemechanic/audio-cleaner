#include <iostream>
#include <vector>
#include <cstdint>
#include <cmath>
#include "../include/audio_loader.h"

// Clipping analyzer to find clipping regions
class ClippingAnalyzer {
public:
    static void analyzeClipping(const std::string& filename) {
        std::cout << "Analyzing clipping in: " << filename << std::endl;
        
        AudioLoader loader;
        std::vector<int16_t> audioData;
        AudioFormat format;
        
        if (!loader.loadAudio(filename, audioData, format)) {
            std::cerr << "Failed to load file" << std::endl;
            return;
        }
        
        const float threshold = 0.95f;
        const int16_t clipThreshold = static_cast<int16_t>(threshold * 32767.0f);
        
        std::vector<std::pair<size_t, size_t>> clippingRegions;
        bool inClippingRegion = false;
        size_t regionStart = 0;
        int totalClippedSamples = 0;
        
        for (size_t i = 0; i < audioData.size(); i++) {
            if (std::abs(audioData[i]) > clipThreshold) {
                totalClippedSamples++;
                if (!inClippingRegion) {
                    inClippingRegion = true;
                    regionStart = i;
                }
            } else {
                if (inClippingRegion) {
                    inClippingRegion = false;
                    clippingRegions.emplace_back(regionStart, i - 1);
                }
            }
        }
        
        // Handle case where file ends while clipping
        if (inClippingRegion) {
            clippingRegions.emplace_back(regionStart, audioData.size() - 1);
        }
        
        std::cout << "Total clipped samples: " << totalClippedSamples << std::endl;
        std::cout << "Clipping regions: " << clippingRegions.size() << std::endl;
        
        for (const auto& region : clippingRegions) {
            double startTime = static_cast<double>(region.first) / format.sampleRate;
            double endTime = static_cast<double>(region.second) / format.sampleRate;
            size_t duration = region.second - region.first;
            
            std::cout << "  Region: " << startTime << "s - " << endTime 
                      << "s (duration: " << duration << " samples)" << std::endl;
            
            // Show some samples from this region
            if (duration > 0 && region.first < audioData.size()) {
                std::cout << "    Samples: ";
                for (size_t i = region.first; i < std::min(region.first + 10, audioData.size()); i++) {
                    std::cout << audioData[i] << " ";
                }
                std::cout << std::endl;
            }
        }
        
        // Find the region around 6:42 (402 seconds)
        double targetTime = 402.0; // 6:42 in seconds
        size_t targetSample = static_cast<size_t>(targetTime * format.sampleRate);
        
        std::cout << "\nLooking for clipping around " << targetTime << "s (sample " << targetSample << "):" << std::endl;
        
        for (const auto& region : clippingRegions) {
            if (region.first <= targetSample && region.second >= targetSample) {
                double startTime = static_cast<double>(region.first) / format.sampleRate;
                double endTime = static_cast<double>(region.second) / format.sampleRate;
                std::cout << "  ✓ Found clipping region: " << startTime << "s - " << endTime << "s" << std::endl;
                
                // Show peak values in this region
                int16_t maxVal = 0;
                int16_t minVal = 0;
                for (size_t i = region.first; i <= region.second && i < audioData.size(); i++) {
                    maxVal = std::max(maxVal, audioData[i]);
                    minVal = std::min(minVal, audioData[i]);
                }
                std::cout << "    Peak range: [" << minVal << ", " << maxVal << "]" << std::endl;
                break;
            }
        }
    }
};

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " <wav_file>" << std::endl;
        return 1;
    }
    
    ClippingAnalyzer::analyzeClipping(argv[1]);
    return 0;
}
