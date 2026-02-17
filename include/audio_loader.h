#pragma once
#include "audio_interface.h"
#include "audio_factory.h"
#include <memory>
#include <string>

class AudioLoader {
private:
    std::unique_ptr<IAudioReader> reader;
    std::unique_ptr<IAudioWriter> writer;
    AudioFormat format;
    
public:
    AudioLoader() = default;
    ~AudioLoader() = default;
    
    bool loadAudio(const std::string& filename, std::vector<int16_t>& audioData, AudioFormat& outFormat);
    bool saveAudio(const std::string& filename, const std::vector<int16_t>& audioData, const AudioFormat& format);
    
    AudioFormat getFormat() const { return format; }
    
    static bool isFormatSupported(const std::string& filename);
    static std::vector<std::string> getSupportedFormats();
    static std::vector<std::string> getSupportedExtensions();
};
