#pragma once
#include "audio_interface.h"
#include <memory>
#include <algorithm>
#include <cstring>
#include <cctype>

// OGG Vorbis format plugin
// In a real implementation, this would use libvorbis/libogg

class OggReader : public IAudioReader {
private:
    void* oggHandle; // Opaque handle to OGG decoder
    AudioFormat format;
    bool isOpen;
    
public:
    OggReader();
    ~OggReader() override;
    
    bool open(const std::string& filename) override;
    bool read(std::vector<int16_t>& audioData) override;
    void close() override;
    
    AudioFormat getFormat() const override { return format; }
    std::string getFormatName() const override { return "OGG Vorbis"; }
    std::vector<std::string> getSupportedExtensions() const override {
        return {"ogg", "oga"};
    }
    bool canHandle(const std::string& filename) const override;
};

class OggWriter : public IAudioWriter {
private:
    void* oggHandle; // Opaque handle to OGG encoder
    AudioFormat format;
    bool isOpen;
    float quality; // 0.0-1.0, 1.0 being highest quality
    
public:
    OggWriter(float quality = 0.7f);
    ~OggWriter() override;
    
    bool open(const std::string& filename, const AudioFormat& format) override;
    bool write(const std::vector<int16_t>& audioData) override;
    void close() override;
    
    std::string getFormatName() const override { return "OGG Vorbis"; }
    std::vector<std::string> getSupportedExtensions() const override {
        return {"ogg", "oga"};
    }
    bool canHandle(const std::string& filename) const override;
    
    void setQuality(float q) { quality = std::max(0.0f, std::min(1.0f, q)); }
};

class OggFormat : public IAudioFormat {
private:
    float defaultQuality;
    
public:
    OggFormat(float quality = 0.7f) : defaultQuality(quality) {}
    
    std::unique_ptr<IAudioReader> createReader() override {
        return std::make_unique<OggReader>();
    }
    
    std::unique_ptr<IAudioWriter> createWriter() override {
        return std::make_unique<OggWriter>(defaultQuality);
    }
    
    std::string getFormatName() const override { return "OGG Vorbis"; }
    
    std::vector<std::string> getSupportedExtensions() const override {
        return {"ogg", "oga"};
    }
    
    bool canHandle(const std::string& filename) const override {
        std::string ext = filename.substr(filename.find_last_of('.') + 1);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        return ext == "ogg" || ext == "oga";
    }
    
    int getPriority() const override { return 7; } // Medium priority
};
