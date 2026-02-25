#include <iostream>
#include <cmath>
#include <chrono>

// FFT Size Analysis Calculator
class FFTAnalyzer {
public:
    static void analyzeFFTSize(int fftSize, int sampleRate = 44100) {
        double frequencyResolution = (double)sampleRate / fftSize;
        double timeResolution = (double)fftSize / sampleRate;
        double processingTime = std::log2(fftSize) * fftSize * 0.000001; // Rough estimate
        
        std::cout << "FFT Size: " << fftSize << " points\n";
        std::cout << "  Frequency Resolution: " << frequencyResolution << " Hz\n";
        std::cout << "  Time Resolution: " << timeResolution * 1000 << " ms\n";
        std::cout << "  Estimated Processing: " << processingTime << " ms per frame\n";
        std::cout << "  Memory per Frame: " << fftSize * 8 << " bytes\n";
        
        // Quality assessment
        if (frequencyResolution < 10) {
            std::cout << "  ✓ Excellent frequency precision\n";
        } else if (frequencyResolution < 50) {
            std::cout << "  ✓ Good frequency precision\n";
        } else if (frequencyResolution < 200) {
            std::cout << "  ⚠ Moderate frequency precision\n";
        } else {
            std::cout << "  ✗ Poor frequency precision\n";
        }
        
        if (timeResolution < 0.01) {
            std::cout << "  ✓ Excellent time precision\n";
        } else if (timeResolution < 0.05) {
            std::cout << "  ✓ Good time precision\n";
        } else if (timeResolution < 0.2) {
            std::cout << "  ⚠ Moderate time precision\n";
        } else {
            std::cout << "  ✗ Poor time precision\n";
        }
        
        std::cout << "\n";
    }
    
    static void compareSizes() {
        std::cout << "FFT Size Comparison for Audio Processing\n";
        std::cout << "========================================\n\n";
        
        int sizes[] = {128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768, 65536};
        
        for (int size : sizes) {
            analyzeFFTSize(size);
        }
        
        std::cout << "Recommendations:\n";
        std::cout << "- 128-256: Real-time processing, low latency\n";
        std::cout << "- 512-1024: Good balance for audio cleaning (current)\n";
        std::cout << "- 2048-4096: High quality, slower processing\n";
        std::cout << "- 8192+: Studio quality, very slow\n";
        std::cout << "- 65536: Research/analysis only, impractical for real use\n";
    }
};

int main() {
    FFTAnalyzer::compareSizes();
    return 0;
}
