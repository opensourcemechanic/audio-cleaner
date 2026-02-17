#include <iostream>
#include <vector>
#include <cmath>
#include <fstream>
#include <random>
#include <algorithm>

// Generate a synthetic reference file for echo cancellation
// This creates a room impulse response or noise profile

class ReferenceGenerator {
private:
    int sampleRate;
    int duration; // in seconds
    
public:
    ReferenceGenerator(int sr = 44100, int dur = 5) : sampleRate(sr), duration(dur) {}
    
    // Generate room impulse response (RIR)
    std::vector<int16_t> generateRoomImpulse() {
        std::vector<int16_t> reference(sampleRate * duration * 2); // stereo
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> noise(-0.1, 0.1); // Low level noise
        
        // Simulate room echo with multiple delayed reflections
        std::vector<std::pair<int, double>> echoes = {
            {0, 1.0},        // Direct sound
            {441, 0.3},      // 10ms delay
            {882, 0.2},      // 20ms delay  
            {1323, 0.15},    // 30ms delay
            {1764, 0.1},     // 40ms delay
            {2205, 0.08},    // 50ms delay
            {2646, 0.05},    // 60ms delay
            {3087, 0.03},    // 70ms delay
            {3528, 0.02},    // 80ms delay
            {3969, 0.01}     // 90ms delay
        };
        
        for (size_t i = 0; i < reference.size(); i += 2) {
            double sample = noise(gen); // Background noise
            
            // Add echo reflections
            for (auto& echo : echoes) {
                int delay = echo.first;
                double amplitude = echo.second;
                
                if (i >= delay * 2) {
                    // Add a small impulse at the echo time
                    if (i == delay * 2) {
                        sample += amplitude * 0.5; // Impulse
                    }
                    // Add reverberation tail
                    sample += amplitude * noise(gen) * 0.2;
                }
            }
            
            // Convert to 16-bit
            int16_t sample16 = static_cast<int16_t>(sample * 32767.0);
            reference[i] = sample16;
            reference[i + 1] = sample16; // Same for both channels
        }
        
        return reference;
    }
    
    // Generate white noise reference (for general echo cancellation)
    std::vector<int16_t> generateWhiteNoise() {
        std::vector<int16_t> reference(sampleRate * duration * 2); // stereo
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> noise(-0.05, 0.05); // Very low level noise
        
        for (size_t i = 0; i < reference.size(); i += 2) {
            double sample = noise(gen);
            int16_t sample16 = static_cast<int16_t>(sample * 32767.0);
            reference[i] = sample16;
            reference[i + 1] = sample16;
        }
        
        return reference;
    }
    
    // Save reference to WAV file
    bool saveToWAV(const std::vector<int16_t>& reference, const std::string& filename) {
        std::ofstream file(filename, std::ios::binary);
        if (!file) return false;
        
        // WAV header
        int sampleCount = reference.size() / 2;
        int byteRate = sampleRate * 2 * 2; // 44100 * 2 channels * 2 bytes
        
        file.write("RIFF", 4);
        int fileSize = 36 + sampleCount * 2 * 2;
        file.write(reinterpret_cast<const char*>(&fileSize), 4);
        file.write("WAVE", 4);
        
        file.write("fmt ", 4);
        int fmtSize = 16;
        file.write(reinterpret_cast<const char*>(&fmtSize), 4);
        short format = 1; // PCM
        file.write(reinterpret_cast<const char*>(&format), 2);
        short channels = 2;
        file.write(reinterpret_cast<const char*>(&channels), 2);
        file.write(reinterpret_cast<const char*>(&sampleRate), 4);
        file.write(reinterpret_cast<const char*>(&byteRate), 4);
        short blockAlign = 4;
        file.write(reinterpret_cast<const char*>(&blockAlign), 2);
        short bitsPerSample = 16;
        file.write(reinterpret_cast<const char*>(&bitsPerSample), 2);
        
        file.write("data", 4);
        int dataSize = sampleCount * 2 * 2;
        file.write(reinterpret_cast<const char*>(&dataSize), 4);
        
        // Write audio data
        file.write(reinterpret_cast<const char*>(reference.data()), reference.size() * 2);
        
        return true;
    }
};

int main(int argc, char* argv[]) {
    std::cout << "Audio Reference File Generator\n";
    std::cout << "=============================\n\n";
    
    std::string type = "impulse"; // Default
    int duration = 5; // Default 5 seconds
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--type" && i + 1 < argc) {
            type = argv[++i];
        } else if (arg == "--duration" && i + 1 < argc) {
            duration = std::stoi(argv[++i]);
        } else if (arg == "--help") {
            std::cout << "Usage: " << argv[0] << " [options]\n\n";
            std::cout << "Options:\n";
            std::cout << "  --type <type>        Reference type: impulse, noise (default: impulse)\n";
            std::cout << "  --duration <sec>     Duration in seconds (default: 5)\n";
            std::cout << "  --help               Show this help\n\n";
            std::cout << "Output files:\n";
            std::cout << "  room_impulse.wav      Room impulse response reference\n";
            std::cout << "  white_noise.wav       White noise reference\n\n";
            std::cout << "Examples:\n";
            std::cout << "  " << argv[0] << " --type impulse --duration 10\n";
            std::cout << "  " << argv[0] << " --type noise --duration 3\n";
            return 0;
        }
    }
    
    ReferenceGenerator gen(44100, duration);
    
    if (type == "impulse") {
        std::cout << "Generating room impulse response reference...\n";
        auto reference = gen.generateRoomImpulse();
        if (gen.saveToWAV(reference, "room_impulse.wav")) {
            std::cout << "✅ Created: room_impulse.wav (" << duration << " seconds)\n";
        }
    } else if (type == "noise") {
        std::cout << "Generating white noise reference...\n";
        auto reference = gen.generateWhiteNoise();
        if (gen.saveToWAV(reference, "white_noise.wav")) {
            std::cout << "✅ Created: white_noise.wav (" << duration << " seconds)\n";
        }
    } else {
        std::cerr << "Error: Unknown type '" << type << "'. Use 'impulse' or 'noise'.\n";
        return 1;
    }
    
    std::cout << "\n📝 How to use with Audio Cleaner:\n";
    std::cout << "   ./audio_cleaner -i echo_audio.wav -r room_impulse.wav -o clean.wav\n";
    std::cout << "   ./audio_cleaner -i echo_audio.wav -r white_noise.wav -o clean.wav\n";
    
    return 0;
}
