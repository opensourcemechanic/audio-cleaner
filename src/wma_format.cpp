#include "../include/wma_format.h"
#include <iostream>
#include <fstream>
#include <algorithm>

// WMA format implementation using FFmpeg

WmaReader::WmaReader() : formatContext(nullptr), codecContext(nullptr), 
                         swrContext(nullptr), audioStreamIndex(-1), isOpen(false) {
}

WmaReader::~WmaReader() {
    close();
}

bool WmaReader::open(const std::string& filename) {
    if (!canHandle(filename)) {
        return false;
    }
    
    // Initialize FFmpeg (only once)
    static bool ffmpegInitialized = false;
    if (!ffmpegInitialized) {
        // av_register_all() is deprecated in newer FFmpeg versions
        ffmpegInitialized = true;
    }
    
    // Open input file
    if (avformat_open_input(&formatContext, filename.c_str(), nullptr, nullptr) != 0) {
        std::cerr << "Error: Cannot open WMA file " << filename << std::endl;
        return false;
    }
    
    // Get stream information
    if (avformat_find_stream_info(formatContext, nullptr) < 0) {
        std::cerr << "Error: Cannot find stream information in " << filename << std::endl;
        avformat_close_input(&formatContext);
        return false;
    }
    
    // Find audio stream
    audioStreamIndex = av_find_best_stream(formatContext, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    if (audioStreamIndex < 0) {
        std::cerr << "Error: No audio stream found in " << filename << std::endl;
        avformat_close_input(&formatContext);
        return false;
    }
    
    // Get codec parameters
    AVCodecParameters* codecParams = formatContext->streams[audioStreamIndex]->codecpar;
    
    // Find decoder
    const AVCodec* codec = avcodec_find_decoder(codecParams->codec_id);
    if (!codec) {
        std::cerr << "Error: Unsupported codec in " << filename << std::endl;
        avformat_close_input(&formatContext);
        return false;
    }
    
    // Allocate codec context
    codecContext = avcodec_alloc_context3(codec);
    if (!codecContext) {
        std::cerr << "Error: Failed to allocate codec context" << std::endl;
        avformat_close_input(&formatContext);
        return false;
    }
    
    // Copy codec parameters
    if (avcodec_parameters_to_context(codecContext, codecParams) < 0) {
        std::cerr << "Error: Failed to copy codec parameters" << std::endl;
        avcodec_free_context(&codecContext);
        avformat_close_input(&formatContext);
        return false;
    }
    
    // Open codec
    if (avcodec_open2(codecContext, codec, nullptr) < 0) {
        std::cerr << "Error: Failed to open codec" << std::endl;
        avcodec_free_context(&codecContext);
        avformat_close_input(&formatContext);
        return false;
    }
    
    // Set up resampler to convert to 16-bit PCM
    int64_t inChannelLayout = (codecContext->channel_layout != 0) ? 
                              codecContext->channel_layout : 
                              av_get_default_channel_layout(codecContext->channels);
    
    swrContext = swr_alloc_set_opts(nullptr,
                                    AV_CH_LAYOUT_STEREO, AV_SAMPLE_FMT_S16, 44100,
                                    inChannelLayout, codecContext->sample_fmt, codecContext->sample_rate,
                                    0, nullptr);
    
    if (!swrContext || swr_init(swrContext) < 0) {
        std::cerr << "Error: Failed to initialize resampler" << std::endl;
        avcodec_free_context(&codecContext);
        avformat_close_input(&formatContext);
        return false;
    }
    
    // Set format information
    format = AudioFormat(44100, 2, 16, 0);
    isOpen = true;
    
    return true;
}

bool WmaReader::read(std::vector<int16_t>& audioData) {
    if (!isOpen) {
        std::cerr << "Error: WMA file not open" << std::endl;
        return false;
    }
    
    AVPacket* packet = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();
    audioData.clear();
    
    std::cout << "Starting to read WMA audio data..." << std::endl;
    
    int totalSamples = 0;
    while (av_read_frame(formatContext, packet) >= 0) {
        if (packet->stream_index == audioStreamIndex) {
            // Send packet to decoder
            int ret = avcodec_send_packet(codecContext, packet);
            if (ret == 0) {
                // Receive decoded frames
                while ((ret = avcodec_receive_frame(codecContext, frame)) == 0) {
                    std::cout << "Decoded frame: " << frame->nb_samples << " samples" << std::endl;
                    
                    // Calculate output samples (44.1kHz stereo)
                    int outSamples = swr_get_out_samples(swrContext, frame->nb_samples);
                    
                    if (outSamples > 0) {
                        // Calculate buffer size
                        int bufferSize = outSamples * 2 * sizeof(int16_t); // stereo * 16-bit
                        uint8_t* buffer = (uint8_t*)av_malloc(bufferSize);
                        uint8_t* outBuffer = buffer;
                        
                        if (buffer) {
                            // Convert samples
                            int convertedSamples = swr_convert(swrContext,
                                                            &outBuffer, outSamples,
                                                            (const uint8_t**)frame->data, frame->nb_samples);
                            
                            if (convertedSamples > 0) {
                                // Copy samples to output vector
                                int16_t* samples = (int16_t*)buffer;
                                size_t oldSize = audioData.size();
                                audioData.resize(oldSize + convertedSamples * 2);
                                memcpy(&audioData[oldSize], samples, convertedSamples * 2 * sizeof(int16_t));
                                totalSamples += convertedSamples * 2;
                                std::cout << "Converted " << convertedSamples * 2 << " samples, total: " << totalSamples << std::endl;
                            }
                            
                            av_free(buffer);
                        }
                    }
                }
                
                if (ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
                    std::cerr << "Error receiving frame: " << ret << std::endl;
                    break;
                }
            } else {
                std::cerr << "Error sending packet: " << ret << std::endl;
            }
        }
        av_packet_unref(packet);
    }
    
    // Flush any remaining samples
    uint8_t* buffer = (uint8_t*)av_malloc(4096 * 2 * sizeof(int16_t));
    if (buffer) {
        int remainingSamples = swr_convert(swrContext, &buffer, 4096, nullptr, 0);
        while (remainingSamples > 0) {
            int16_t* samples = (int16_t*)buffer;
            size_t oldSize = audioData.size();
            audioData.resize(oldSize + remainingSamples * 2);
            memcpy(&audioData[oldSize], samples, remainingSamples * 2 * sizeof(int16_t));
            totalSamples += remainingSamples * 2;
            std::cout << "Flushed " << remainingSamples * 2 << " samples, total: " << totalSamples << std::endl;
            
            remainingSamples = swr_convert(swrContext, &buffer, 4096, nullptr, 0);
        }
        av_free(buffer);
    }
    
    std::cout << "Finished reading WMA, total samples: " << totalSamples << std::endl;
    
    av_frame_free(&frame);
    av_packet_free(&packet);
    
    return totalSamples > 0;
}

void WmaReader::close() {
    if (swrContext) {
        swr_free(&swrContext);
        swrContext = nullptr;
    }
    
    if (codecContext) {
        avcodec_free_context(&codecContext);
        codecContext = nullptr;
    }
    
    if (formatContext) {
        avformat_close_input(&formatContext);
        formatContext = nullptr;
    }
    
    isOpen = false;
}

bool WmaReader::canHandle(const std::string& filename) const {
    std::string ext = filename.substr(filename.find_last_of('.') + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext == "wma";
}

// WMA Writer implementation (simplified - WMA encoding is complex)

WmaWriter::WmaWriter() : formatContext(nullptr), codecContext(nullptr),
                         swrContext(nullptr), isOpen(false) {
}

WmaWriter::~WmaWriter() {
    close();
}

bool WmaWriter::open(const std::string& filename, const AudioFormat& fmt) {
    if (!canHandle(filename)) {
        return false;
    }
    
    std::cerr << "Warning: WMA encoding is not fully implemented in this version" << std::endl;
    std::cerr << "WMA files can be read but not written. Please output to WAV or MP3 format." << std::endl;
    return false;
}

bool WmaWriter::write(const std::vector<int16_t>& audioData) {
    std::cerr << "Error: WMA encoding not implemented" << std::endl;
    return false;
}

void WmaWriter::close() {
    if (swrContext) {
        swr_free(&swrContext);
        swrContext = nullptr;
    }
    
    if (codecContext) {
        avcodec_free_context(&codecContext);
        codecContext = nullptr;
    }
    
    if (formatContext) {
        avformat_free_context(formatContext);
        formatContext = nullptr;
    }
    
    isOpen = false;
}

bool WmaWriter::canHandle(const std::string& filename) const {
    std::string ext = filename.substr(filename.find_last_of('.') + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext == "wma";
}
