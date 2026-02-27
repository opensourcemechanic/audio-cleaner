#include "../include/wav_format.h"
#include <fstream>
#include <iostream>
#include <cstring>
#include <algorithm>

// Helper functions for little-endian reading (WAV files are little-endian)
uint16_t readLE16(std::ifstream& file) {
    uint8_t bytes[2];
    file.read(reinterpret_cast<char*>(bytes), 2);
    return bytes[0] | (bytes[1] << 8);
}

uint32_t readLE32(std::ifstream& file) {
    uint8_t bytes[4];
    file.read(reinterpret_cast<char*>(bytes), 4);
    return bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24);
}

// Helper functions for little-endian writing
void writeLE16(std::ofstream& file, uint16_t value) {
    uint8_t bytes[2] = {static_cast<uint8_t>(value & 0xFF), static_cast<uint8_t>((value >> 8) & 0xFF)};
    file.write(reinterpret_cast<char*>(bytes), 2);
}

void writeLE32(std::ofstream& file, uint32_t value) {
    uint8_t bytes[4] = {
        static_cast<uint8_t>(value & 0xFF),
        static_cast<uint8_t>((value >> 8) & 0xFF),
        static_cast<uint8_t>((value >> 16) & 0xFF),
        static_cast<uint8_t>((value >> 24) & 0xFF)
    };
    file.write(reinterpret_cast<char*>(bytes), 4);
}

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
    fileSize = readLE32(file);
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
        chunkSize = readLE32(file);
        
        if (strncmp(chunkId, "fmt ", 4) == 0) {
            // Read format chunk
            header.audioFormat = readLE16(file);
            header.numChannels = readLE16(file);
            header.sampleRate = readLE32(file);
            header.byteRate = readLE32(file);
            header.blockAlign = readLE16(file);
            header.bitsPerSample = readLE16(file);
            
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
        chunkSize = readLE32(file);
        
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
    
    // Write placeholder header using little-endian functions
    file.write("RIFF", 4);
    writeLE32(file, 0); // fileSize placeholder
    file.write("WAVE", 4);
    file.write("fmt ", 4);
    writeLE32(file, 16); // fmtSize
    writeLE16(file, 1); // audioFormat (PCM)
    writeLE16(file, format.numChannels);
    writeLE32(file, format.sampleRate);
    writeLE32(file, format.sampleRate * format.numChannels * format.bitsPerSample / 8); // byteRate
    writeLE16(file, format.numChannels * format.bitsPerSample / 8); // blockAlign
    writeLE16(file, format.bitsPerSample);
    file.write("data", 4);
    writeLE32(file, 0); // dataSize placeholder
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
uint32_t dataSize = static_cast<std::streamoff>(currentPos) - 44; // 44 bytes for standard WAV header
uint32_t fileSize = static_cast<std::streamoff>(currentPos) - 8;

// Go back and update the size fields
file.seekp(4);  // Position after "RIFF"
writeLE32(file, fileSize);
file.seekp(40); // Position after "data"
writeLE32(file, dataSize);

file.close();
}

bool WavWriter::canHandle(const std::string& filename) const {
std::string ext = filename.substr(filename.find_last_of('.') + 1);
std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
return ext == "wav" || ext == "wave";
}
