#include "../include/wav_reader.h"
#include <fstream>
#include <iostream>
#include <cstring>

bool WavReader::load(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open file " << filename << std::endl;
        return false;
    }
    
    file.read(reinterpret_cast<char*>(&header), sizeof(WavHeader));
    
    if (strncmp(header.riff, "RIFF", 4) != 0 || strncmp(header.wave, "WAVE", 4) != 0) {
        std::cerr << "Error: Invalid WAV file format" << std::endl;
        return false;
    }
    
    if (header.bitsPerSample != 16) {
        std::cerr << "Error: Only 16-bit PCM supported" << std::endl;
        return false;
    }
    
    audioData.resize(header.dataSize / sizeof(int16_t));
    file.read(reinterpret_cast<char*>(audioData.data()), header.dataSize);
    
    file.close();
    return true;
}

bool WavReader::save(const std::string& filename) {
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot create file " << filename << std::endl;
        return false;
    }
    
    header.dataSize = audioData.size() * sizeof(int16_t);
    header.fileSize = sizeof(WavHeader) - 8 + header.dataSize;
    
    file.write(reinterpret_cast<char*>(&header), sizeof(WavHeader));
    file.write(reinterpret_cast<char*>(audioData.data()), header.dataSize);
    
    file.close();
    return true;
}

void WavReader::setAudioData(const std::vector<int16_t>& data) {
    audioData = data;
}

void WavReader::clear() {
    audioData.clear();
}
