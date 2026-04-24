#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include "meta_reader.h"

// Frequency-domain magnitude for a block of samples
struct SpectrumFrame {
    std::vector<float> magnitudes; // fftSize/2+1 bins
};

struct WavData {
    std::vector<int16_t> samples;
    int sample_rate = 44100;
    int channels    = 1;
};

class AudioVizApp {
public:
    AudioVizApp(const std::string& originalPath,
                const std::string& cleanedPath,
                const std::string& metaPath,
                bool pipeMode = false);
    ~AudioVizApp();

    void run();

private:
    // Data
    VizMetadata   meta_;
    WavData       original_;
    WavData       cleaned_;

    bool          pipeMode_;
    bool          playbackActive_  = false;
    int           playbackSample_  = 0;   // current sample position in original_

    // Derived data
    std::vector<float> diffSamples_;           // cleaned - original (time domain)
    std::vector<float> origFloat_;             // original in float [-1,1]
    std::vector<float> cleanFloat_;            // cleaned  in float [-1,1]

    // Per-quiet-section spectra (for freq domain panels)
    struct SectionSpectra {
        std::vector<float> beforeMag;
        std::vector<float> afterMag;
        std::vector<float> diffMag;
        std::vector<float> freqAxis; // Hz per bin
    };
    std::vector<SectionSpectra> sectionSpectra_;

    // Global spectra (averaged over quiet sections)
    std::vector<float> avgNoiseSpectrum_;
    std::vector<float> freqAxis_;

    // miniaudio playback handle (opaque ptr to avoid including miniaudio.h here)
    void* maEngine_ = nullptr;
    void* maSound_  = nullptr;

    // Helpers
    bool loadWav(const std::string& path, WavData& out);
    void computeDerived();
    void computeSectionSpectra();
    void computeFFT(const std::vector<float>& samples, std::vector<float>& mag);

    void initPlayback();
    void shutdownPlayback();
    void pipeOutput();

    void renderUI();
    void renderNoisePanel();
    void renderBeforeAfterPanel();
    void renderDiffPanel();
    void renderPlaybackBar();
};
