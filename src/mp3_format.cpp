#include "../include/mp3_format.h"
#include <iostream>
#include <fstream>
#include <algorithm>

#ifdef HAVE_MPG123
#include <mpg123.h>
#endif
#ifdef HAVE_LAME
#include <lame/lame.h>
#endif

// Real MP3 implementation using libmpg123 (reading) and LAME (writing)

#ifdef HAVE_MPG123
static bool mpg123_initialized = false;
#endif

#ifdef HAVE_MPG123
Mp3Reader::Mp3Reader() : mp3Handle(nullptr), isOpen(false) {
    if (!mpg123_initialized) {
        mpg123_init();
        mpg123_initialized = true;
    }
    mp3Handle = mpg123_new(nullptr, nullptr);
}

Mp3Reader::~Mp3Reader() {
    close();
    if (mp3Handle) {
        mpg123_delete(mp3Handle);
        mp3Handle = nullptr;
    }
}

bool Mp3Reader::open(const std::string& filename) {
    if (!canHandle(filename)) {
        return false;
    }
    
    if (mpg123_open(mp3Handle, filename.c_str()) != MPG123_OK) {
        std::cerr << "Error: Cannot open MP3 file " << filename << std::endl;
        return false;
    }
    
    long rate;
    int channels, encoding;
    if (mpg123_getformat(mp3Handle, &rate, &channels, &encoding) != MPG123_OK) {
        std::cerr << "Error: Cannot get MP3 format from " << filename << std::endl;
        mpg123_close(mp3Handle);
        return false;
    }
    
    // Set format to 16-bit signed PCM for output
    const int* encodings;
    size_t encoding_count;
    mpg123_encodings(&encodings, &encoding_count);
    
    bool supported = false;
    for (size_t i = 0; i < encoding_count; i++) {
        if (encodings[i] == MPG123_ENC_SIGNED_16) {
            supported = true;
            break;
        }
    }
    
    if (supported) {
        mpg123_format(mp3Handle, rate, channels, MPG123_ENC_SIGNED_16);
    } else {
        std::cerr << "Error: 16-bit signed PCM not supported" << std::endl;
        mpg123_close(mp3Handle);
        return false;
    }
    
    format = AudioFormat(rate, channels, 16, 0);
    isOpen = true;
    return true;
}

bool Mp3Reader::read(std::vector<int16_t>& audioData) {
    if (!isOpen) {
        std::cerr << "Error: MP3 file not open" << std::endl;
        return false;
    }
    
    const size_t bufferSize = 4096;
    unsigned char buffer[bufferSize];
    size_t bytesRead;
    
    audioData.clear();
    
    while (true) {
        int result = mpg123_read(mp3Handle, buffer, bufferSize, &bytesRead);
        
        if (bytesRead > 0) {
            // Convert bytes to int16_t samples
            size_t samples = bytesRead / sizeof(int16_t);
            const int16_t* samplesPtr = reinterpret_cast<const int16_t*>(buffer);
            audioData.insert(audioData.end(), samplesPtr, samplesPtr + samples);
        }
        
        if (result == MPG123_DONE) {
            break;
        } else if (result != MPG123_OK) {
            std::cerr << "Error: MP3 decoding failed" << std::endl;
            return false;
        }
    }
    
    return true;
}

void Mp3Reader::close() {
    if (isOpen && mp3Handle) {
        mpg123_close(mp3Handle);
    }
    isOpen = false;
}

bool Mp3Reader::canHandle(const std::string& filename) const {
    std::string ext = filename.substr(filename.find_last_of('.') + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext == "mp3";
}
#endif // HAVE_MPG123

#ifdef HAVE_LAME
Mp3Writer::Mp3Writer(int quality) : lameHandle(nullptr), isOpen(false), quality(quality), filename("") {}

Mp3Writer::~Mp3Writer() {
    close();
}

bool Mp3Writer::open(const std::string& filename, const AudioFormat& fmt) {
    if (!canHandle(filename)) {
        return false;
    }
    
    format = fmt;
    this->filename = filename;
    
    // Initialize LAME encoder
    lameHandle = lame_init();
    if (!lameHandle) {
        std::cerr << "Error: Failed to initialize LAME encoder" << std::endl;
        return false;
    }
    
    // Set LAME parameters
    lame_set_in_samplerate(lameHandle, fmt.sampleRate);
    lame_set_num_channels(lameHandle, fmt.numChannels);
    lame_set_out_samplerate(lameHandle, fmt.sampleRate);
    lame_set_brate(lameHandle, 128); // 128 kbps bitrate
    lame_set_mode(lameHandle, fmt.numChannels == 1 ? MONO : JOINT_STEREO);
    lame_set_quality(lameHandle, quality); // 0-9, 0 = best quality
    
    if (lame_init_params(lameHandle) < 0) {
        std::cerr << "Error: Failed to initialize LAME parameters" << std::endl;
        lame_close(lameHandle);
        lameHandle = nullptr;
        return false;
    }
    
    // Open output file
    file.open(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot create MP3 file " << filename << std::endl;
        lame_close(lameHandle);
        lameHandle = nullptr;
        return false;
    }
    
    isOpen = true;
    return true;
}

bool Mp3Writer::write(const std::vector<int16_t>& audioData) {
    if (!isOpen || !lameHandle) {
        std::cerr << "Error: MP3 file not open for writing" << std::endl;
        return false;
    }
    
    const int bufferSize = 4096;
    unsigned char mp3Buffer[bufferSize];
    
    if (format.numChannels == 1) {
        // Mono
        const int16_t* samples = audioData.data();
        size_t samplesProcessed = 0;
        
        while (samplesProcessed < audioData.size()) {
            int samplesToProcess = std::min(bufferSize / 2, static_cast<int>(audioData.size() - samplesProcessed));
            
            int bytesWritten = lame_encode_buffer(lameHandle, 
                const_cast<int16_t*>(samples + samplesProcessed), // left channel
                nullptr, // right channel (null for mono)
                samplesToProcess,
                mp3Buffer, bufferSize);
            
            if (bytesWritten < 0) {
                std::cerr << "Error: MP3 encoding failed" << std::endl;
                return false;
            }
            
            if (bytesWritten > 0) {
                file.write(reinterpret_cast<const char*>(mp3Buffer), bytesWritten);
            }
            
            samplesProcessed += samplesToProcess;
        }
    } else {
        // Stereo
        std::vector<int16_t> leftChannel, rightChannel;
        leftChannel.reserve(audioData.size() / 2);
        rightChannel.reserve(audioData.size() / 2);
        
        // De-interleave stereo data
        for (size_t i = 0; i < audioData.size(); i += 2) {
            leftChannel.push_back(audioData[i]);
            rightChannel.push_back(audioData[i + 1]);
        }
        
        const int16_t* leftSamples = leftChannel.data();
        const int16_t* rightSamples = rightChannel.data();
        size_t framesProcessed = 0;
        size_t totalFrames = leftChannel.size();
        
        while (framesProcessed < totalFrames) {
            int framesToProcess = std::min(bufferSize / 4, static_cast<int>(totalFrames - framesProcessed));
            
            int bytesWritten = lame_encode_buffer(lameHandle,
                const_cast<int16_t*>(leftSamples + framesProcessed), // left channel
                const_cast<int16_t*>(rightSamples + framesProcessed), // right channel
                framesToProcess,
                mp3Buffer, bufferSize);
            
            if (bytesWritten < 0) {
                std::cerr << "Error: MP3 encoding failed" << std::endl;
                return false;
            }
            
            if (bytesWritten > 0) {
                file.write(reinterpret_cast<const char*>(mp3Buffer), bytesWritten);
            }
            
            framesProcessed += framesToProcess;
        }
    }
    
    return true;
}

void Mp3Writer::close() {
    if (isOpen && lameHandle) {
        // Flush remaining MP3 data
        const int bufferSize = 4096;
        unsigned char mp3Buffer[bufferSize];
        
        int bytesWritten = lame_encode_flush(lameHandle, mp3Buffer, bufferSize);
        if (bytesWritten > 0 && file.is_open()) {
            file.write(reinterpret_cast<const char*>(mp3Buffer), bytesWritten);
        }
        
        lame_close(lameHandle);
        lameHandle = nullptr;
        
        if (file.is_open()) {
            file.close();
        }
    }
    isOpen = false;
}

bool Mp3Writer::canHandle(const std::string& filename) const {
    std::string ext = filename.substr(filename.find_last_of('.') + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext == "mp3";
}
#endif // HAVE_LAME
