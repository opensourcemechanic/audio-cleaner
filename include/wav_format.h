#pragma once
#include "audio_interface.h"
#include <fstream>
#include <algorithm>
#include <cstring>
#include <cctype>

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

class WavReader : public IAudioReader {
private:
    std::ifstream file;
    WavHeader header;
    AudioFormat format;
    
public:
    WavReader() = default;
    ~WavReader() override { close(); }
    
    bool open(const std::string& filename) override;
    bool read(std::vector<int16_t>& audioData) override;
    void close() override;
    
    AudioFormat getFormat() const override { return format; }
    std::string getFormatName() const override { return "WAV"; }
    std::vector<std::string> getSupportedExtensions() const override {
        return {"wav", "wave"};
    }
    bool canHandle(const std::string& filename) const override;
};

class WavWriter : public IAudioWriter {
private:
    std::ofstream file;
    AudioFormat format;
    std::string file_name;
    
public:
    WavWriter() = default;
    ~WavWriter() override { close(); }
    
    bool open(const std::string& filename, const AudioFormat& format) override;
    bool write(const std::vector<int16_t>& audioData) override;
    void close() override;
    
    std::string getFormatName() const override { return "WAV"; }
    std::vector<std::string> getSupportedExtensions() const override {
        return {"wav", "wave"};
    }
    bool canHandle(const std::string& filename) const override;
};

class WavFormat : public IAudioFormat {
public:
    std::unique_ptr<IAudioReader> createReader() override {
        return std::make_unique<WavReader>();
    }
    
    std::unique_ptr<IAudioWriter> createWriter() override {
        return std::make_unique<WavWriter>();
    }
    
    std::string getFormatName() const override { return "WAV"; }
    
    std::vector<std::string> getSupportedExtensions() const override {
        return {"wav", "wave"};
    }
    
    bool canHandle(const std::string& filename) const override {
        std::string ext = filename.substr(filename.find_last_of('.') + 1);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        return ext == "wav" || ext == "wave";
    }
    
    int getPriority() const override { return 10; } // High priority for WAV
};
