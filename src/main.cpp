#include "../include/audio_loader.h"
#include "../include/audio_processor.h"
#include "../include/audio_processor_backend.h"
#include "../include/wav_format.h"

#ifdef HAVE_MPG123
#ifdef HAVE_LAME
#include "../include/mp3_format.h"
#endif
#endif

#ifdef HAVE_VORBIS
#ifdef HAVE_OGG
#include "../include/ogg_format.h"
#endif
#endif

#ifdef HAVE_FFMPEG
#include "../include/wma_format.h"
#endif
#include <iostream>
#include <chrono>

void initializeFormats() {
    auto& factory = AudioFormatFactory::getInstance();
    
    // Register built-in formats
    factory.registerFormat(std::make_unique<WavFormat>());
    
#ifdef HAVE_MPG123
#ifdef HAVE_LAME
    factory.registerFormat(std::make_unique<Mp3Format>());
#endif
#endif

#ifdef HAVE_VORBIS
#ifdef HAVE_OGG
    factory.registerFormat(std::make_unique<OggFormat>());
#endif
#endif

#ifdef HAVE_FFMPEG
    factory.registerFormat(std::make_unique<WmaFormat>());
#endif
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
    std::cout << "  --fft-size <size>   FFT size for spectral analysis (128-65536, power of 2, default: 1024)\n";
    std::cout << "  --reduce-clipping   Smooth clipped audio peaks (default threshold: 95%)\n";
    std::cout << "  --clipping-threshold <threshold> Clipping threshold (0.8-0.99, default: 0.95)\n";
    std::cout << "  --force-gpu         Force GPU acceleration (requires OpenCL)\n";
    std::cout << "  --backend <type>    Force processing backend (cpu/opencl/auto)\n";
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
    std::cout << "  " << programName << " -i noisy.wav -o clean.wav --fft-size 512\n";
    std::cout << "    # Faster processing with 512-point FFT\n\n";
    std::cout << "  " << programName << " -i noisy.wav -o clean.wav --fft-size 4096\n";
    std::cout << "    # Higher quality with 4096-point FFT\n\n";
    std::cout << "  " << programName << " -i noisy.wav -o clean.wav --fft-size 65536\n";
    std::cout << "    # Studio quality with 65536-point FFT (very slow)\n\n";
    std::cout << "  " << programName << " -i clipped.wav -o smooth.wav --reduce-clipping\n";
    std::cout << "    # Smooth clipped audio peaks to reduce distortion\n\n";
    std::cout << "  " << programName << " -i heavily_clipped.wav -o smooth.wav --reduce-clipping --clipping-threshold 0.9\n";
    std::cout << "    # More aggressive clipping reduction at 90% threshold\n\n";
    std::cout << "  " << programName << " -i long_audio.wav -o clean.wav --force-gpu\n";
    std::cout << "    # Force GPU acceleration for faster processing\n\n";
    std::cout << "  " << programName << " -i audio.wav -o clean.wav --backend opencl\n";
    std::cout << "    # Use OpenCL backend explicitly\n\n";
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
    int fftSize = 1024;
    bool enableClippingReduction = false;
    float clippingThreshold = 0.95f;
    bool forceGPU = false;
    std::string backendType = "auto";
    
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
        else if (arg == "--fft-size" && i + 1 < argc) {
            fftSize = std::stoi(argv[++i]);
            // Validate FFT size (must be power of 2 and between 64 and 65536)
            if (fftSize < 64 || fftSize > 65536 || (fftSize & (fftSize - 1)) != 0) {
                std::cerr << "Warning: Invalid FFT size " << fftSize << ", using 1024" << std::endl;
                fftSize = 1024;
            }
        }
        else if (arg == "--reduce-clipping") {
            enableClippingReduction = true;
        }
        else if (arg == "--clipping-threshold" && i + 1 < argc) {
            clippingThreshold = std::stof(argv[++i]);
            // Validate threshold (must be between 0.8 and 0.99)
            if (clippingThreshold < 0.8f || clippingThreshold > 0.99f) {
                std::cerr << "Warning: Invalid clipping threshold " << clippingThreshold << ", using 0.95" << std::endl;
                clippingThreshold = 0.95f;
            }
        }
        else if (arg == "--force-gpu") {
            forceGPU = true;
        }
        else if (arg == "--backend" && i + 1 < argc) {
            backendType = argv[++i];
            // Validate backend type
            if (backendType != "cpu" && backendType != "opencl" && backendType != "auto") {
                std::cerr << "Warning: Invalid backend type " << backendType << ", using auto" << std::endl;
                backendType = "auto";
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
    
    // Calculate audio duration for smart backend selection
    double audioDuration = static_cast<double>(audioData.size()) / (inputFormat.sampleRate * inputFormat.numChannels);
    
    // Create optimal backend based on duration and user preferences
    std::unique_ptr<AudioProcessorBackend> backend;
    AudioProcessorFactory::BackendType selectedBackend = AudioProcessorFactory::BackendType::AUTO;
    
    if (backendType == "cpu") {
        selectedBackend = AudioProcessorFactory::BackendType::CPU;
    } else if (backendType == "opencl") {
        selectedBackend = AudioProcessorFactory::BackendType::OPENCL;
    }
    
    backend = AudioProcessorFactory::createOptimalBackend(audioDuration, forceGPU);
    
    std::cout << "\n=== PROCESSING BACKEND ===\n";
    std::cout << "   • Backend: " << backend->getBackendName();
    if (backend->isGPUAccelerated()) {
        std::cout << " (GPU Accelerated)";
    }
    std::cout << "\n";
    std::cout << "   • Device: " << backend->getDeviceInfo() << "\n";
    std::cout << "   • Audio Duration: " << audioDuration << " seconds (" << (audioDuration / 60.0) << " minutes)\n";
    
    if (audioDuration > 300.0) {
        std::cout << "   • GPU acceleration: Auto-enabled for long audio (>5 minutes)\n";
    } else if (forceGPU) {
        std::cout << "   • GPU acceleration: Forced by user\n";
    } else {
        std::cout << "   • GPU acceleration: Not needed for short audio\n";
    }
    std::cout << "\n";
    
    AudioProcessor processor(fftSize, std::move(backend));
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
    }
    
    // Apply low-frequency removal if enabled
    if (enableLowFreqRemoval) {
        processor.processLowFrequencyRemoval(audioData, lowFreqCutoff);
    }
    
    // Apply normalization if enabled
    if (enableNormalization) {
        processor.processNormalization(audioData, normalizeLevel);
    }
    
    // Apply clipping reduction if enabled
    if (enableClippingReduction) {
        processor.processClippingReduction(audioData, clippingThreshold);
    }
    
    std::cout << "\n=== PROCESSING DETAILS ===\n";
    std::cout << "   • FFT Size: " << fftSize << " samples with 75% overlap\n";
    std::cout << "   • Frequency Resolution: " << (44100.0f / fftSize) << " Hz per bin\n";
    std::cout << "   • Time Resolution: " << (fftSize / 44100.0f * 1000) << " ms per frame\n";
    std::cout << "   • Window Function: Hann window\n";
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
