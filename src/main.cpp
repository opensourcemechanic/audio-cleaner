#include "../include/audio_loader.h"
#include "../include/audio_processor.h"
#include "../include/wav_format.h"
#include "../include/mp3_format.h"
#include "../include/ogg_format.h"
#include "../include/wma_format.h"
#include <iostream>
#include <chrono>

void initializeFormats() {
    auto& factory = AudioFormatFactory::getInstance();
    
    // Register built-in formats
    factory.registerFormat(std::make_unique<WavFormat>());
    factory.registerFormat(std::make_unique<Mp3Format>());
    factory.registerFormat(std::make_unique<OggFormat>());
    factory.registerFormat(std::make_unique<WmaFormat>());
}

void printUsage(const char* programName) {
    std::cout << "Audio Cleaner - Echo Cancellation and Noise Reduction\n";
    std::cout << "===============================================\n\n";
    std::cout << "This tool cleans audio files using advanced signal processing:\n";
    std::cout << "\n🔧 NOISE REDUCTION (always applied):\n";
    std::cout << "   • Removes background hiss, hum, and stationary noise\n";
    std::cout << "   • Uses spectral subtraction with FFT analysis\n";
    std::cout << "   • Best for: fan noise, electrical hum, tape hiss\n\n";
    std::cout << "🔧 ECHO CANCELLATION (optional, requires reference file):\n";
    std::cout << "   • Removes echo/reverb using adaptive LMS filtering\n";
    std::cout << "   • Reference signal captures the echo characteristics\n";
    std::cout << "   • Best for: room echo, telephone echo, conference calls\n\n";
    std::cout << "🔧 WMA SUPPORT (read-only):\n";
    std::cout << "   • Reads Windows Media Audio files using FFmpeg\n";
    std::cout << "   • Converts WMA to PCM for processing\n";
    std::cout << "   • Note: WMA files can be read but not written (use WAV/MP3 output)\n\n";
    std::cout << "Usage: " << programName << " [options]\n\n";
    std::cout << "Options:\n";
    std::cout << "  -i <input_file>     Input audio file (required)\n";
    std::cout << "  -o <output_file>    Output audio file (required)\n";
    std::cout << "  -r <reference_file> Reference signal for echo cancellation (optional)\n";
    std::cout << "  -l <learning_rate>  Learning rate for adaptive filter (0.001-0.1, default: 0.01)\n";
    std::cout << "  --low-freq <hz>     Remove low-frequency noise below specified Hz (20-500, default: 80)\n";
    std::cout << "  --normalize <db>    Normalize audio to target level in dBFS (-60 to 0, default: -6)\n";
    std::cout << "  -f                  List supported formats\n";
    std::cout << "  -h                  Show this help\n\n";
    std::cout << "Supported formats: ";
    for (const auto& ext : AudioLoader::getSupportedExtensions()) {
        std::cout << "." << ext << " ";
    }
    std::cout << "\n\n";
    std::cout << "Examples:\n";
    std::cout << "  " << programName << " -i noisy.wav -o clean.wav\n";
    std::cout << "    # Noise reduction only\n\n";
    std::cout << "  " << programName << " -i echo_call.wav -r reference.wav -o clean.wav\n";
    std::cout << "    # Echo cancellation + noise reduction\n\n";
    std::cout << "  " << programName << " -i noisy.mp3 -o clean.wav -l 0.005\n";
    std::cout << "    # Noise reduction with slower adaptation rate\n\n";
    std::cout << "  " << programName << " -i breathing_noise.wav -o clean.wav --low-freq 80\n";
    std::cout << "    # Remove low-frequency breathing/rumble noise\n\n";
    std::cout << "  " << programName << " -i quiet_audio.wav -o normalized.wav --normalize\n";
    std::cout << "    # Normalize to standard audio level (-6 dBFS)\n\n";
    std::cout << "  " << programName << " -i podcast.wav -o loud.wav --normalize -3\n";
    std::cout << "    # Normalize to hot master level (-3 dBFS)\n\n";
    std::cout << "  " << programName << " -i background.wav -o gentle.wav --normalize -12\n";
    std::cout << "    # Normalize to conservative level (-12 dBFS)\n\n";
    std::cout << "  " << programName << " -i audio.wma -o cleaned.wav --low-freq 80\n";
    std::cout << "    # Process WMA file and output to WAV\n\n";
    std::cout << "Technical Details:\n";
    std::cout << "  • FFT Size: 1024 samples with 75% overlap\n";
    std::cout << "  • Window: Hann window for smooth transitions\n";
    std::cout << "  • Echo Cancellation: Least Mean Squares (LMS) adaptive filter\n";
    std::cout << "  • Noise Reduction: Spectral subtraction with flooring\n\n";
}

int main(int argc, char* argv[]) {
    // Initialize audio format plugins
    initializeFormats();
    
    std::string inputFile, outputFile, referenceFile;
    float learningRate = 0.01f;
    float lowFreqCutoff = 0.0f; // 0 = disabled
    bool enableLowFreqRemoval = false;
    float normalizeLevel = 0.0f; // 0 = disabled
    bool enableNormalization = false;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            printUsage(argv[0]);
            return 0;
        }
        else if (arg == "-f" || arg == "--formats") {
            std::cout << "Supported audio formats:\n";
            for (const auto& format : AudioLoader::getSupportedFormats()) {
                std::cout << "  " << format << "\n";
            }
            std::cout << "\nSupported file extensions:\n";
            for (const auto& ext : AudioLoader::getSupportedExtensions()) {
                std::cout << "  ." << ext << "\n";
            }
            return 0;
        }
        else if (arg == "-i" && i + 1 < argc) {
            inputFile = argv[++i];
        }
        else if (arg == "-o" && i + 1 < argc) {
            outputFile = argv[++i];
        }
        else if (arg == "-r" && i + 1 < argc) {
            referenceFile = argv[++i];
        }
        else if (arg == "-l" && i + 1 < argc) {
            learningRate = std::stof(argv[++i]);
            learningRate = std::max(0.001f, std::min(0.1f, learningRate));
        }
        else if (arg == "--low-freq" && i + 1 < argc) {
            lowFreqCutoff = std::stof(argv[++i]);
            enableLowFreqRemoval = true;
            // Validate cutoff range (20 Hz - 500 Hz)
            lowFreqCutoff = std::max(20.0f, std::min(500.0f, lowFreqCutoff));
        }
        else if (arg == "--normalize") {
            if (i + 1 < argc && argv[i + 1][0] != '-') {
                // Next argument is a number, use it as target level
                normalizeLevel = std::stof(argv[++i]);
                enableNormalization = true;
                // Validate level range (-60 to 0 dB)
                normalizeLevel = std::max(-60.0f, std::min(0.0f, normalizeLevel));
            } else {
                // No value provided, use default -6dB
                normalizeLevel = -6.0f;
                enableNormalization = true;
            }
        }
    }
    
    if (inputFile.empty() || outputFile.empty()) {
        std::cerr << "Error: Input and output files are required\n";
        printUsage(argv[0]);
        return 1;
    }
    
    if (!AudioLoader::isFormatSupported(inputFile)) {
        std::cerr << "Error: Input format not supported: " << inputFile << std::endl;
        std::cerr << "Use -f to see supported formats" << std::endl;
        return 1;
    }
    
    if (!AudioLoader::isFormatSupported(outputFile)) {
        std::cerr << "Error: Output format not supported: " << outputFile << std::endl;
        std::cerr << "Use -f to see supported formats" << std::endl;
        return 1;
    }
    
    std::cout << "Loading audio files...\n";
    
    AudioLoader loader;
    std::vector<int16_t> audioData, referenceData;
    AudioFormat inputFormat, referenceFormat;
    
    if (!loader.loadAudio(inputFile, audioData, inputFormat)) {
        std::cerr << "Failed to load input file: " << inputFile << std::endl;
        return 1;
    }
    
    bool hasReference = false;
    if (!referenceFile.empty()) {
        AudioLoader refLoader;
        if (!refLoader.loadAudio(referenceFile, referenceData, referenceFormat)) {
            std::cerr << "Warning: Failed to load reference file: " << referenceFile << std::endl;
            std::cerr << "Continuing with noise reduction only...\n";
        } else {
            if (referenceFormat.sampleRate != inputFormat.sampleRate ||
                referenceFormat.numChannels != inputFormat.numChannels) {
                std::cerr << "Warning: Reference file format mismatch, continuing with noise reduction only...\n";
            } else {
                hasReference = true;
                std::cout << "Reference signal loaded for echo cancellation\n";
            }
        }
    }
    
    std::cout << "Input audio: " << audioData.size() / inputFormat.numChannels << " samples, ";
    std::cout << inputFormat.sampleRate << " Hz, ";
    std::cout << inputFormat.numChannels << " channels, ";
    std::cout << "Format: " << loader.getFormat().bitsPerSample << "-bit\n";
    
    AudioProcessor processor;
    processor.setLearningRate(learningRate);
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    std::cout << "Processing audio...\n";
    std::cout << "\n=== AUDIO CLEANING ALGORITHMS ===\n\n";
    
    if (hasReference) {
        std::cout << "🔧 ECHO CANCELLATION + NOISE REDUCTION:\n";
        std::cout << "   • Echo Cancellation: Using adaptive LMS filter to model and remove echo\n";
        std::cout << "     - Reference signal helps identify echo components\n";
        std::cout << "     - Learning rate: " << learningRate << " (controls adaptation speed)\n";
        std::cout << "     - Continuously adjusts filter coefficients to match echo path\n";
        std::cout << "\n";
        std::cout << "   • Noise Reduction: Spectral subtraction algorithm\n";
        std::cout << "     - Analyzes audio in frequency domain using FFT\n";
        std::cout << "     - Estimates background noise spectrum from quiet segments\n";
        std::cout << "     - Subtracts noise estimate while preserving speech/music\n";
        std::cout << "     - Applies spectral flooring to prevent musical noise artifacts\n";
        std::cout << "\n";
        processor.processFull(audioData, referenceData);
        std::cout << "✅ Applied: Echo cancellation + Noise reduction\n";
    } else {
        std::cout << "🔧 NOISE REDUCTION ONLY:\n";
        std::cout << "   • Spectral Subtraction Algorithm:\n";
        std::cout << "     - Converts audio to frequency domain using 1024-point FFT\n";
        std::cout << "     - Uses 75% overlap windowing for smooth transitions\n";
        std::cout << "     - Estimates noise profile from first audio frames\n";
        std::cout << "     - Subtracts estimated noise from frequency spectrum\n";
        std::cout << "     - Applies Hann window to reduce spectral artifacts\n";
        std::cout << "     - Converts back to time domain with overlap-add\n";
        std::cout << "\n";
        std::cout << "   • Best for: Stationary background noise (hiss, hum, fan noise)\n";
        std::cout << "   • Limitations: Less effective for rapidly changing noise\n";
        std::cout << "\n";
        processor.processNoiseReduction(audioData);
        std::cout << "✅ Applied: Noise reduction only\n";
    }
    
    // Apply low-frequency removal if enabled
    if (enableLowFreqRemoval) {
        processor.processLowFrequencyRemoval(audioData, lowFreqCutoff);
    }
    
    // Apply normalization if enabled
    if (enableNormalization) {
        processor.processNormalization(audioData, normalizeLevel);
    }
    
    std::cout << "\n=== PROCESSING DETAILS ===\n";
    std::cout << "   • FFT Size: 1024 samples\n";
    std::cout << "   • Window Function: Hann window\n";
    std::cout << "   • Overlap: 75% (256-sample hop size)\n";
    std::cout << "   • Sample Resolution: 16-bit PCM\n";
    std::cout << "\n";
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    std::cout << "Processing completed in " << duration.count() << " ms\n";
    
    if (!loader.saveAudio(outputFile, audioData, inputFormat)) {
        std::cerr << "Failed to save output file: " << outputFile << std::endl;
        return 1;
    }
    
    std::cout << "Cleaned audio saved to: " << outputFile << std::endl;
    
    return 0;
}
