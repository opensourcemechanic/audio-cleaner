#pragma once
#include <vector>
#include <string>
#include <cstdint>

struct WavHeader {
    char riff[4];
    uint32_t fileSize;
    char wave[4];
    char fmt[4];
    uint32_t fmtSize;
    uint16_t audioFormat;
    uint16_t numChannels;
    uint32_t sampleRate;
    uint32_t byteRate;
    uint16_t blockAlign;
    uint16_t bitsPerSample;
    char data[4];
    uint32_t dataSize;
};

class WavReader {
private:
    WavHeader header;
    std::vector<int16_t> audioData;
    
public:
    bool load(const std::string& filename);
    bool save(const std::string& filename);
    
    std::vector<int16_t>& getAudioData() { return audioData; }
    const std::vector<int16_t>& getAudioData() const { return audioData; }
    
    uint16_t getNumChannels() const { return header.numChannels; }
    uint32_t getSampleRate() const { return header.sampleRate; }
    uint16_t getBitsPerSample() const { return header.bitsPerSample; }
    uint32_t getNumSamples() const { return audioData.size() / header.numChannels; }
    
    void setAudioData(const std::vector<int16_t>& data);
    void clear();
};
