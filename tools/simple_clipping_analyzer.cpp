#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>
#include <cmath>

// Simple WAV clipping analyzer
class SimpleClippingAnalyzer {
public:
    static void analyzeClipping(const std::string& filename) {
        std::cout << "Analyzing clipping in: " << filename << std::endl;
        
        std::ifstream file(filename, std::ios::binary);
        if (!file) {
            std::cerr << "Failed to open file" << std::endl;
            return;
        }
        
        // Skip WAV header (44 bytes)
        file.seekg(44);
        
        const float threshold = 0.95f;
        const int16_t clipThreshold = static_cast<int16_t>(threshold * 32767.0f);
        const int sampleRate = 44100;
        
        std::vector<std::pair<size_t, size_t>> clippingRegions;
        bool inClippingRegion = false;
        size_t regionStart = 0;
        int totalClippedSamples = 0;
        size_t sampleCount = 0;
        
        int16_t sample;
        while (file.read(reinterpret_cast<char*>(&sample), sizeof(int16_t))) {
            if (std::abs(sample) > clipThreshold) {
                totalClippedSamples++;
                if (!inClippingRegion) {
                    inClippingRegion = true;
                    regionStart = sampleCount;
                }
            } else {
                if (inClippingRegion) {
                    inClippingRegion = false;
                    clippingRegions.emplace_back(regionStart, sampleCount - 1);
                }
            }
            sampleCount++;
        }
        
        // Handle case where file ends while clipping
        if (inClippingRegion) {
            clippingRegions.emplace_back(regionStart, sampleCount - 1);
        }
        
        std::cout << "Total samples: " << sampleCount << std::endl;
        std::cout << "Total clipped samples: " << totalClippedSamples << std::endl;
        std::cout << "Clipping regions: " << clippingRegions.size() << std::endl;
        
        // Show first few clipping regions
        int regionsToShow = std::min(5, (int)clippingRegions.size());
        for (int i = 0; i < regionsToShow; i++) {
            const auto& region = clippingRegions[i];
            double startTime = static_cast<double>(region.first) / sampleRate;
            double endTime = static_cast<double>(region.second) / sampleRate;
            size_t duration = region.second - region.first;
            
            std::cout << "  Region " << (i+1) << ": " << startTime << "s - " << endTime 
                      << "s (duration: " << duration << " samples)" << std::endl;
        }
        
        // Find the region around 6:42 (402 seconds)
        double targetTime = 402.0; // 6:42 in seconds
        size_t targetSample = static_cast<size_t>(targetTime * sampleRate);
        
        std::cout << "\nLooking for clipping around " << targetTime << "s (sample " << targetSample << "):" << std::endl;
        
        bool foundTarget = false;
        for (const auto& region : clippingRegions) {
            if (region.first <= targetSample && region.second >= targetSample) {
                double startTime = static_cast<double>(region.first) / sampleRate;
                double endTime = static_cast<double>(region.second) / sampleRate;
                std::cout << "  ✓ Found clipping region: " << startTime << "s - " << endTime << "s" << std::endl;
                foundTarget = true;
                break;
            }
        }
        
        if (!foundTarget) {
            std::cout << "  No clipping found exactly at 6:42" << std::endl;
            
            // Find closest region
            double closestDistance = 999999.0;
            size_t closestRegion = 0;
            
            for (size_t i = 0; i < clippingRegions.size(); i++) {
                const auto& region = clippingRegions[i];
                double regionCenter = static_cast<double>(region.first + region.second) / 2.0 / sampleRate;
                double distance = std::abs(regionCenter - targetTime);
                
                if (distance < closestDistance) {
                    closestDistance = distance;
                    closestRegion = i;
                }
            }
            
            if (!clippingRegions.empty()) {
                const auto& region = clippingRegions[closestRegion];
                double startTime = static_cast<double>(region.first) / sampleRate;
                double endTime = static_cast<double>(region.second) / sampleRate;
                std::cout << "  Closest region: " << startTime << "s - " << endTime 
                          << "s (distance: " << closestDistance << "s)" << std::endl;
            }
        }
        
        std::cout << "\nClipping percentage: " << (static_cast<double>(totalClippedSamples) / sampleCount * 100.0) << "%" << std::endl;
    }
};

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " <wav_file>" << std::endl;
        return 1;
    }
    
    SimpleClippingAnalyzer::analyzeClipping(argv[1]);
    return 0;
}
