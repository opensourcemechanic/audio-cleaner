#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <fstream>
#include <stdexcept>
#include "nlohmann/json.hpp"

struct VizQuietSection {
    float time_ms;
    int frame_index;
    float energy;
    std::vector<int16_t> before_samples;
    std::vector<int16_t> after_samples;
};

struct VizMetadata {
    std::string generator;
    int version = 0;
    std::string command_line;
    std::string input_file;
    std::string output_file;
    int sample_rate = 44100;
    int channels = 1;
    int bits_per_sample = 16;
    int fft_size = 512;
    float alpha = 0.3f;
    float beta = 0.3f;
    float normalize_level_db = 0.0f;
    bool has_normalize = false;
    std::vector<VizQuietSection> quiet_sections;
    std::vector<float> noise_spectrum;
};

inline VizMetadata loadMetadata(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) throw std::runtime_error("Cannot open metadata file: " + path);

    nlohmann::json j;
    f >> j;

    VizMetadata m;
    m.generator       = j.value("generator", "");
    m.version         = j.value("version", 0);
    m.command_line    = j.value("command_line", "");
    m.input_file      = j.value("input_file", "");
    m.output_file     = j.value("output_file", "");
    m.sample_rate     = j.value("sample_rate", 44100);
    m.channels        = j.value("channels", 1);
    m.bits_per_sample = j.value("bits_per_sample", 16);
    m.fft_size        = j.value("fft_size", 512);
    m.alpha           = j.value("alpha", 0.3f);
    m.beta            = j.value("beta", 0.3f);
    if (j.contains("normalize_level_db")) {
        m.normalize_level_db = j["normalize_level_db"].get<float>();
        m.has_normalize = true;
    }

    m.noise_spectrum = j.value("noise_spectrum", std::vector<float>{});

    if (j.contains("quiet_sections")) {
        for (const auto& qs : j["quiet_sections"]) {
            VizQuietSection s;
            s.time_ms     = qs.value("time_ms", 0.0f);
            s.frame_index = qs.value("frame_index", 0);
            s.energy      = qs.value("energy", 0.0f);
            s.before_samples = qs.value("before_samples", std::vector<int16_t>{});
            s.after_samples  = qs.value("after_samples",  std::vector<int16_t>{});
            m.quiet_sections.push_back(std::move(s));
        }
    }

    return m;
}

// Derive .json sidecar path from audio file path
inline std::string deriveMetaPath(const std::string& audioPath) {
    size_t dot = audioPath.rfind('.');
    return (dot != std::string::npos ? audioPath.substr(0, dot) : audioPath) + ".json";
}
