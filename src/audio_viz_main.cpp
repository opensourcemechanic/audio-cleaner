#include "../include/audio_viz_app.h"
#include "../include/meta_reader.h"
#include <iostream>
#include <string>
#include <filesystem>

static void printUsage(const char* prog) {
    std::cout << "Usage: " << prog << " [options] <original.wav> <cleaned.wav> [meta.json]\n\n";
    std::cout << "Options:\n";
    std::cout << "  --pipe          Write original audio to stdout as raw S16LE (for piping to aplay)\n";
    std::cout << "  -h, --help      Show this help\n\n";
    std::cout << "If meta.json is omitted, it is derived from <cleaned.wav> (same path, .json extension).\n\n";
    std::cout << "Examples:\n";
    std::cout << "  " << prog << " original.wav cleaned.wav\n";
    std::cout << "  " << prog << " original.wav cleaned.wav processing.json\n";
    std::cout << "  " << prog << " --pipe original.wav cleaned.wav | aplay -f S16_LE -r 44100 -c 2\n";
}

int main(int argc, char* argv[]) {
    std::string originalPath, cleanedPath, metaPath;
    bool pipeMode = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            printUsage(argv[0]);
            return 0;
        } else if (arg == "--pipe") {
            pipeMode = true;
        } else if (originalPath.empty()) {
            originalPath = arg;
        } else if (cleanedPath.empty()) {
            cleanedPath = arg;
        } else if (metaPath.empty()) {
            metaPath = arg;
        }
    }

    if (originalPath.empty() || cleanedPath.empty()) {
        std::cerr << "Error: original and cleaned audio files are required.\n\n";
        printUsage(argv[0]);
        return 1;
    }

    // Auto-derive meta path from cleaned file if not provided
    if (metaPath.empty()) {
        metaPath = deriveMetaPath(cleanedPath);
        // Fall back to deriving from original if cleaned.json doesn't exist
        if (!std::filesystem::exists(metaPath)) {
            std::string altMeta = deriveMetaPath(originalPath);
            if (std::filesystem::exists(altMeta)) {
                metaPath = altMeta;
                std::cout << "Using metadata: " << metaPath << "\n";
            } else {
                std::cerr << "Error: Could not find metadata file.\n"
                          << "  Tried: " << metaPath << "\n"
                          << "  Tried: " << altMeta << "\n"
                          << "Pass the .json path explicitly as the third argument.\n";
                return 1;
            }
        }
    }

    try {
        AudioVizApp app(originalPath, cleanedPath, metaPath, pipeMode);
        app.run();
    } catch (const std::exception& e) {
        std::cerr << "audio_viz error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
