#include "../include/audio_loader.h"
#include <iostream>

bool AudioLoader::loadAudio(const std::string& filename, std::vector<int16_t>& audioData, AudioFormat& outFormat) {
    auto& factory = AudioFormatFactory::getInstance();
    reader = factory.createReader(filename);
    
    if (!reader) {
        std::cerr << "Error: No reader available for file: " << filename << std::endl;
        return false;
    }
    
    if (!reader->open(filename)) {
        std::cerr << "Error: Failed to open file: " << filename << std::endl;
        return false;
    }
    
    if (!reader->read(audioData)) {
        std::cerr << "Error: Failed to read audio data from: " << filename << std::endl;
        reader->close();
        return false;
    }
    
    outFormat = reader->getFormat();
    format = outFormat;
    reader->close();
    
    return true;
}

bool AudioLoader::saveAudio(const std::string& filename, const std::vector<int16_t>& audioData, const AudioFormat& format) {
    auto& factory = AudioFormatFactory::getInstance();
    writer = factory.createWriter(filename);
    
    if (!writer) {
        std::cerr << "Error: No writer available for file: " << filename << std::endl;
        return false;
    }
    
    if (!writer->open(filename, format)) {
        std::cerr << "Error: Failed to create file: " << filename << std::endl;
        return false;
    }
    
    if (!writer->write(audioData)) {
        std::cerr << "Error: Failed to write audio data to: " << filename << std::endl;
        writer->close();
        return false;
    }
    
    writer->close();
    return true;
}

bool AudioLoader::isFormatSupported(const std::string& filename) {
    auto& factory = AudioFormatFactory::getInstance();
    return factory.isFormatSupported(filename);
}

std::vector<std::string> AudioLoader::getSupportedFormats() {
    auto& factory = AudioFormatFactory::getInstance();
    return factory.getSupportedFormats();
}

std::vector<std::string> AudioLoader::getSupportedExtensions() {
    auto& factory = AudioFormatFactory::getInstance();
    return factory.getSupportedExtensions();
}
