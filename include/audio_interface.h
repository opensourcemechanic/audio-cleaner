#pragma once
#include <vector>
#include <string>
#include <cstdint>
#include <memory>

struct AudioFormat {
    uint32_t sampleRate;
    uint16_t numChannels;
    uint16_t bitsPerSample;
    uint64_t totalSamples;
    
    AudioFormat() : sampleRate(44100), numChannels(2), bitsPerSample(16), totalSamples(0) {}
    AudioFormat(uint32_t sr, uint16_t nc, uint16_t bps, uint64_t ts) 
        : sampleRate(sr), numChannels(nc), bitsPerSample(bps), totalSamples(ts) {}
};

class IAudioReader {
public:
    virtual ~IAudioReader() = default;
    
    virtual bool open(const std::string& filename) = 0;
    virtual bool read(std::vector<int16_t>& audioData) = 0;
    virtual void close() = 0;
    
    virtual AudioFormat getFormat() const = 0;
    virtual std::string getFormatName() const = 0;
    virtual std::vector<std::string> getSupportedExtensions() const = 0;
    virtual bool canHandle(const std::string& filename) const = 0;
};

class IAudioWriter {
public:
    virtual ~IAudioWriter() = default;
    
    virtual bool open(const std::string& filename, const AudioFormat& format) = 0;
    virtual bool write(const std::vector<int16_t>& audioData) = 0;
    virtual void close() = 0;
    
    virtual std::string getFormatName() const = 0;
    virtual std::vector<std::string> getSupportedExtensions() const = 0;
    virtual bool canHandle(const std::string& filename) const = 0;
};

class IAudioFormat {
public:
    virtual ~IAudioFormat() = default;
    
    virtual std::unique_ptr<IAudioReader> createReader() = 0;
    virtual std::unique_ptr<IAudioWriter> createWriter() = 0;
    virtual std::string getFormatName() const = 0;
    virtual std::vector<std::string> getSupportedExtensions() const = 0;
    virtual bool canHandle(const std::string& filename) const = 0;
    virtual int getPriority() const { return 0; } // Higher priority for format detection
};
