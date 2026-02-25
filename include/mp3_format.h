#pragma once
#include "audio_interface.h"
#include <memory>
#include <algorithm>
#include <cstring>
#include <cctype>
#include <fstream>

// Conditional includes for MP3 support
#ifdef HAVE_MPG123
#include <mpg123.h>
#endif
#ifdef HAVE_LAME
#include <lame/lame.h>
#endif

// MP3 format plugin using libmpg123 (reading) and LAME (writing)

#ifdef HAVE_MPG123
class Mp3Reader : public IAudioReader {
private:
    mpg123_handle* mp3Handle; // libmpg123 handle
    AudioFormat format;
    bool isOpen;
    
public:
    Mp3Reader();
    ~Mp3Reader() override;
    
    bool open(const std::string& filename) override;
    bool read(std::vector<int16_t>& audioData) override;
    void close() override;
    
    AudioFormat getFormat() const override { return format; }
    std::string getFormatName() const override { return "MP3"; }
    std::vector<std::string> getSupportedExtensions() const override {
        return {"mp3"};
    }
    bool canHandle(const std::string& filename) const override;
};
#endif // HAVE_MPG123

#ifdef HAVE_LAME
class Mp3Writer : public IAudioWriter {
private:
    lame_t lameHandle;
    AudioFormat format;
    bool isOpen;
    int quality; // 0-9, 0 being highest quality
    std::string filename;
    std::ofstream file;
    
public:
    Mp3Writer(int quality = 2);
    ~Mp3Writer() override;
    
    bool open(const std::string& filename, const AudioFormat& format) override;
    bool write(const std::vector<int16_t>& audioData) override;
    void close() override;
    
    std::string getFormatName() const override { return "MP3"; }
    std::vector<std::string> getSupportedExtensions() const override {
        return {"mp3"};
    }
    bool canHandle(const std::string& filename) const override;
    
    void setQuality(int q) { quality = std::max(0, std::min(9, q)); }
};
#endif // HAVE_LAME

#if defined(HAVE_MPG123) && defined(HAVE_LAME)
class Mp3Format : public IAudioFormat {
private:
    int defaultQuality;
    
public:
    Mp3Format(int quality = 2) : defaultQuality(quality) {}
    
    std::unique_ptr<IAudioReader> createReader() override {
        return std::make_unique<Mp3Reader>();
    }
    
    std::unique_ptr<IAudioWriter> createWriter() override {
        return std::make_unique<Mp3Writer>(defaultQuality);
    }
    
    std::string getFormatName() const override { return "MP3"; }
    
    std::vector<std::string> getSupportedExtensions() const override {
        return {"mp3"};
    }
    
    bool canHandle(const std::string& filename) const override {
        std::string ext = filename.substr(filename.find_last_of('.') + 1);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        return ext == "mp3";
    }
    
    int getPriority() const override { return 8; } // Medium-high priority
};
#endif // HAVE_MPG123 && HAVE_LAME
