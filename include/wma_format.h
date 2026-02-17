#pragma once
#include "audio_interface.h"
#include <memory>
#include <algorithm>
#include <cstring>
#include <cctype>
#include <fstream>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libswresample/swresample.h>
}

// WMA format plugin using FFmpeg

class WmaReader : public IAudioReader {
private:
    AVFormatContext* formatContext;
    AVCodecContext* codecContext;
    SwrContext* swrContext;
    int audioStreamIndex;
    AudioFormat format;
    bool isOpen;
    
public:
    WmaReader();
    ~WmaReader() override;
    
    bool open(const std::string& filename) override;
    bool read(std::vector<int16_t>& audioData) override;
    void close() override;
    
    AudioFormat getFormat() const override { return format; }
    std::string getFormatName() const override { return "WMA"; }
    std::vector<std::string> getSupportedExtensions() const override {
        return {"wma"};
    }
    bool canHandle(const std::string& filename) const override;
};

class WmaWriter : public IAudioWriter {
private:
    AVFormatContext* formatContext;
    AVCodecContext* codecContext;
    SwrContext* swrContext;
    AudioFormat format;
    bool isOpen;
    std::string filename;
    
public:
    WmaWriter();
    ~WmaWriter() override;
    
    bool open(const std::string& filename, const AudioFormat& format) override;
    bool write(const std::vector<int16_t>& audioData) override;
    void close() override;
    
    std::string getFormatName() const override { return "WMA"; }
    std::vector<std::string> getSupportedExtensions() const override {
        return {"wma"};
    }
    bool canHandle(const std::string& filename) const override;
};

class WmaFormat : public IAudioFormat {
private:
    
public:
    WmaFormat() {}
    
    std::unique_ptr<IAudioReader> createReader() override {
        return std::make_unique<WmaReader>();
    }
    
    std::unique_ptr<IAudioWriter> createWriter() override {
        return std::make_unique<WmaWriter>();
    }
    
    std::string getFormatName() const override { return "WMA"; }
    
    std::vector<std::string> getSupportedExtensions() const override {
        return {"wma"};
    }
    
    bool canHandle(const std::string& filename) const override {
        std::string ext = filename.substr(filename.find_last_of('.') + 1);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        return ext == "wma";
    }
    
    int getPriority() const override { return 6; } // Medium priority
};
