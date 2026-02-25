#include "../include/wav_format.h"
#include <iostream>
#include <cstring>
#include <algorithm>

bool WavReader::open(const std::string& filename) {
    file.open(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open WAV file " << filename << std::endl;
        return false;
    }
    
    // Read RIFF header
    char riff[4], wave[4];
    uint32_t fileSize;
    file.read(riff, 4);
    file.read(reinterpret_cast<char*>(&fileSize), 4);
    file.read(wave, 4);
    
    if (strncmp(riff, "RIFF", 4) != 0 || strncmp(wave, "WAVE", 4) != 0) {
        std::cerr << "Error: Invalid WAV file format in " << filename << std::endl;
        close();
        return false;
    }
    
    // Read chunks until we find the format chunk
    char chunkId[4];
    uint32_t chunkSize;
    bool foundFormat = false;
    
    while (!foundFormat && file.read(chunkId, 4)) {
        file.read(reinterpret_cast<char*>(&chunkSize), 4);
        
        if (strncmp(chunkId, "fmt ", 4) == 0) {
            // Read format chunk
            file.read(reinterpret_cast<char*>(&header.audioFormat), 2);
            file.read(reinterpret_cast<char*>(&header.numChannels), 2);
            file.read(reinterpret_cast<char*>(&header.sampleRate), 4);
            file.read(reinterpret_cast<char*>(&header.byteRate), 4);
            file.read(reinterpret_cast<char*>(&header.blockAlign), 2);
            file.read(reinterpret_cast<char*>(&header.bitsPerSample), 2);
            
            // Skip any remaining format bytes
            if (chunkSize > 16) {
                file.seekg(chunkSize - 16, std::ios::cur);
            }
            
            foundFormat = true;
            break;
        } else {
            // Skip this chunk
            file.seekg(chunkSize, std::ios::cur);
        }
    }
    
    if (!foundFormat) {
        std::cerr << "Error: Could not find format chunk in WAV file" << std::endl;
        close();
        return false;
    }
    
    if (header.bitsPerSample != 16) {
        std::cerr << "Error: Only 16-bit PCM WAV files are supported" << std::endl;
        close();
        return false;
    }
    
    // Now find the data chunk
    bool foundData = false;
    while (!foundData && file.read(chunkId, 4)) {
        file.read(reinterpret_cast<char*>(&chunkSize), 4);
        
        if (strncmp(chunkId, "data", 4) == 0) {
            header.dataSize = chunkSize;
            foundData = true;
            break;
        } else {
            // Skip this chunk
            file.seekg(chunkSize, std::ios::cur);
        }
    }
    
    if (!foundData) {
        std::cerr << "Error: Could not find data chunk in WAV file" << std::endl;
        close();
        return false;
    }
    
    // Store current position for reading
    dataPosition = file.tellg();
    
    format = AudioFormat(header.sampleRate, header.numChannels, header.bitsPerSample, 
                        header.dataSize / (header.bitsPerSample / 8) / header.numChannels);
    
    return true;
}

bool WavReader::read(std::vector<int16_t>& audioData) {
    if (!file.is_open()) {
        std::cerr << "Error: WAV file not open" << std::endl;
        return false;
    }
    
    // Seek to data position
    file.seekg(dataPosition);
    
    size_t samplesToRead = header.dataSize / sizeof(int16_t);
    audioData.resize(samplesToRead);
    
    file.read(reinterpret_cast<char*>(audioData.data()), header.dataSize);
    
    if (file.gcount() != static_cast<std::streamsize>(header.dataSize)) {
        std::cerr << "Warning: Could not read all audio data" << std::endl;
        audioData.resize(file.gcount() / sizeof(int16_t));
    }
    
    return true;
}

void WavReader::close() {
    if (file.is_open()) {
        file.close();
    }
}

bool WavReader::canHandle(const std::string& filename) const {
    std::string ext = filename.substr(filename.find_last_of('.') + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext == "wav" || ext == "wave";
}

bool WavWriter::open(const std::string& filename, const AudioFormat& fmt) {
    if (fmt.bitsPerSample != 16) {
        std::cerr << "Error: Only 16-bit PCM WAV files are supported for writing" << std::endl;
        return false;
    }
    
    format = fmt;
    file_name = filename;
    file.open(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot create WAV file " << filename << std::endl;
        return false;
    }
    
    // Write placeholder header
    WavHeader header;
    strncpy(header.riff, "RIFF", 4);
    strncpy(header.wave, "WAVE", 4);
    strncpy(header.fmt, "fmt ", 4);
    strncpy(header.data, "data", 4);
    
    header.fmtSize = 16;
    header.audioFormat = 1; // PCM
    header.numChannels = format.numChannels;
    header.sampleRate = format.sampleRate;
    header.bitsPerSample = format.bitsPerSample;
    header.blockAlign = header.numChannels * header.bitsPerSample / 8;
    header.byteRate = header.sampleRate * header.blockAlign;
    header.dataSize = 0; // Will be updated when closing
    header.fileSize = 0; // Will be updated when closing
    
    file.write(reinterpret_cast<char*>(&header), sizeof(WavHeader));
    return true;
}

bool WavWriter::write(const std::vector<int16_t>& audioData) {
    if (!file.is_open()) {
        std::cerr << "Error: WAV file not open for writing" << std::endl;
        return false;
    }
    
    file.write(reinterpret_cast<const char*>(audioData.data()), 
              audioData.size() * sizeof(int16_t));
    
    return file.good();
}

void WavWriter::close() {
    if (!file.is_open()) return;
    
    // Update header with actual sizes
    std::streampos currentPos = file.tellp();
    uint32_t dataSize = static_cast<std::streamoff>(currentPos) - sizeof(WavHeader);
    uint32_t fileSize = static_cast<std::streamoff>(currentPos) - 8;
    
    // Go back to beginning and read existing header
    file.seekp(0);
    WavHeader header;
    // We need to read what we already wrote, so temporarily switch to read mode
    file.close();
    std::ifstream inFile(file_name, std::ios::binary);
    if (inFile.is_open()) {
        inFile.read(reinterpret_cast<char*>(&header), sizeof(WavHeader));
        inFile.close();
    }
    
    // Update header fields
    header.dataSize = dataSize;
    header.fileSize = fileSize;
    
    // Reopen for writing and update header
    file.open(file_name, std::ios::binary | std::ios::in | std::ios::out);
    if (file.is_open()) {
        file.seekp(0);
        file.write(reinterpret_cast<char*>(&header), sizeof(WavHeader));
        file.close();
    }
}

bool WavWriter::canHandle(const std::string& filename) const {
    std::string ext = filename.substr(filename.find_last_of('.') + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext == "wav" || ext == "wave";
}
