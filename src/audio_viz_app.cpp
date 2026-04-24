#include "../include/audio_viz_app.h"
#include "../include/meta_reader.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "implot.h"

#include <GLFW/glfw3.h>

#include "../tools/miniaudio.h"

#include <cmath>
#include <complex>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <cstring>

static const float PI = 3.14159265359f;

// ---------------------------------------------------------------------------
// Simple WAV loader (16-bit PCM only, little-endian)
// ---------------------------------------------------------------------------
static uint16_t readLE16(std::ifstream& f) {
    uint8_t b[2]; f.read(reinterpret_cast<char*>(b), 2);
    return static_cast<uint16_t>(b[0] | (b[1] << 8));
}
static uint32_t readLE32(std::ifstream& f) {
    uint8_t b[4]; f.read(reinterpret_cast<char*>(b), 4);
    return static_cast<uint32_t>(b[0]|(b[1]<<8)|(b[2]<<16)|(b[3]<<24));
}

bool AudioVizApp::loadWav(const std::string& path, WavData& out) {
    std::ifstream f(path, std::ios::binary);
    if (!f.is_open()) { std::cerr << "Cannot open WAV: " << path << "\n"; return false; }

    char riff[4]; f.read(riff, 4);
    if (std::strncmp(riff, "RIFF", 4) != 0) { std::cerr << "Not RIFF: " << path << "\n"; return false; }
    readLE32(f); // chunk size
    char wave[4]; f.read(wave, 4);
    if (std::strncmp(wave, "WAVE", 4) != 0) { std::cerr << "Not WAVE: " << path << "\n"; return false; }

    uint16_t numChannels = 1, bitsPerSample = 16;
    uint32_t sampleRate = 44100, dataSize = 0;

    while (f.good()) {
        char id[4]; f.read(id, 4);
        uint32_t sz = readLE32(f);
        if (!f.good()) break;
        if (std::strncmp(id, "fmt ", 4) == 0) {
            readLE16(f); // audio format
            numChannels   = readLE16(f);
            sampleRate    = readLE32(f);
            readLE32(f); readLE16(f); // byte rate, block align
            bitsPerSample = readLE16(f);
            if (sz > 16) f.seekg(sz - 16, std::ios::cur);
        } else if (std::strncmp(id, "data", 4) == 0) {
            dataSize = sz;
            break;
        } else {
            f.seekg(sz, std::ios::cur);
        }
    }

    if (bitsPerSample != 16) {
        std::cerr << "audio_viz: only 16-bit PCM WAV supported (" << path << " is " << bitsPerSample << "-bit)\n";
        return false;
    }

    size_t numSamples = dataSize / 2;
    out.samples.resize(numSamples);
    f.read(reinterpret_cast<char*>(out.samples.data()), numSamples * 2);
    out.sample_rate = static_cast<int>(sampleRate);
    out.channels    = static_cast<int>(numChannels);
    return true;
}

// ---------------------------------------------------------------------------
// Cooley-Tukey FFT (in-place, power-of-2)
// ---------------------------------------------------------------------------
static void fftInPlace(std::vector<std::complex<float>>& a) {
    size_t n = a.size();
    for (size_t i = 1, j = 0; i < n; ++i) {
        size_t bit = n >> 1;
        for (; j & bit; bit >>= 1) j ^= bit;
        j ^= bit;
        if (i < j) std::swap(a[i], a[j]);
    }
    for (size_t len = 2; len <= n; len <<= 1) {
        float ang = -2.0f * PI / len;
        std::complex<float> wlen(cosf(ang), sinf(ang));
        for (size_t i = 0; i < n; i += len) {
            std::complex<float> w(1, 0);
            for (size_t j = 0; j < len / 2; ++j) {
                auto u = a[i + j], v = a[i + j + len/2] * w;
                a[i + j] = u + v;
                a[i + j + len/2] = u - v;
                w *= wlen;
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Constructor / destructor
// ---------------------------------------------------------------------------
AudioVizApp::AudioVizApp(const std::string& originalPath,
                         const std::string& cleanedPath,
                         const std::string& metaPath,
                         bool pipeMode)
    : pipeMode_(pipeMode)
{
    meta_ = loadMetadata(metaPath);

    if (!loadWav(originalPath, original_))
        throw std::runtime_error("Failed to load original: " + originalPath);
    if (!loadWav(cleanedPath, cleaned_))
        throw std::runtime_error("Failed to load cleaned: " + cleanedPath);

    computeDerived();
    computeSectionSpectra();

    if (!pipeMode_)
        initPlayback();
}

AudioVizApp::~AudioVizApp() {
    shutdownPlayback();
}

// ---------------------------------------------------------------------------
// Derived data
// ---------------------------------------------------------------------------
void AudioVizApp::computeDerived() {
    size_t n = std::min(original_.samples.size(), cleaned_.samples.size());
    origFloat_.resize(n);
    cleanFloat_.resize(n);
    diffSamples_.resize(n);

    for (size_t i = 0; i < n; ++i) {
        origFloat_[i]  = original_.samples[i]  / 32768.0f;
        cleanFloat_[i] = cleaned_.samples[i]   / 32768.0f;
        diffSamples_[i] = cleanFloat_[i] - origFloat_[i];
    }
}

void AudioVizApp::computeFFT(const std::vector<float>& samples, std::vector<float>& mag) {
    size_t n = samples.size();
    // Next power-of-2
    size_t fftN = 1; while (fftN < n) fftN <<= 1;

    std::vector<std::complex<float>> buf(fftN, {0,0});
    // Apply Hann window
    for (size_t i = 0; i < n; ++i) {
        float w = 0.5f * (1.0f - cosf(2.0f * PI * i / (fftN - 1)));
        buf[i] = {samples[i] * w, 0};
    }
    fftInPlace(buf);

    size_t half = fftN / 2 + 1;
    mag.resize(half);
    for (size_t i = 0; i < half; ++i)
        mag[i] = std::abs(buf[i]) / fftN;
}

void AudioVizApp::computeSectionSpectra() {
    int fftSz = meta_.fft_size > 0 ? meta_.fft_size : 512;
    int sr    = meta_.sample_rate > 0 ? meta_.sample_rate : 44100;

    size_t half = fftSz / 2 + 1;

    // Build frequency axis (Hz)
    freqAxis_.resize(half);
    for (size_t i = 0; i < half; ++i)
        freqAxis_[i] = static_cast<float>(i) * sr / fftSz;

    // Average noise spectrum (from metadata)
    avgNoiseSpectrum_ = meta_.noise_spectrum;
    if (avgNoiseSpectrum_.size() != half)
        avgNoiseSpectrum_.resize(half, 0.0f);

    // Per quiet section
    sectionSpectra_.resize(meta_.quiet_sections.size());
    for (size_t s = 0; s < meta_.quiet_sections.size(); ++s) {
        const auto& qs = meta_.quiet_sections[s];
        auto& ss = sectionSpectra_[s];
        ss.freqAxis = freqAxis_;

        // Convert before/after int16 to float
        size_t len = std::min(qs.before_samples.size(), qs.after_samples.size());
        std::vector<float> bef(len), aft(len), dif(len);
        for (size_t i = 0; i < len; ++i) {
            bef[i] = qs.before_samples[i] / 32768.0f;
            aft[i] = qs.after_samples[i]  / 32768.0f;
            dif[i] = aft[i] - bef[i];
        }
        computeFFT(bef, ss.beforeMag);
        computeFFT(aft, ss.afterMag);
        computeFFT(dif, ss.diffMag);
    }
}

// ---------------------------------------------------------------------------
// Playback (miniaudio)
// ---------------------------------------------------------------------------
void AudioVizApp::initPlayback() {
    auto* engine = new ma_engine();
    if (ma_engine_init(nullptr, engine) != MA_SUCCESS) {
        std::cerr << "audio_viz: miniaudio engine init failed (no playback)\n";
        delete engine;
        return;
    }
    maEngine_ = engine;
}

void AudioVizApp::shutdownPlayback() {
    if (maSound_) {
        ma_sound_uninit(static_cast<ma_sound*>(maSound_));
        delete static_cast<ma_sound*>(maSound_);
        maSound_ = nullptr;
    }
    if (maEngine_) {
        ma_engine_uninit(static_cast<ma_engine*>(maEngine_));
        delete static_cast<ma_engine*>(maEngine_);
        maEngine_ = nullptr;
    }
}

// ---------------------------------------------------------------------------
// Pipe output: write original + cleaned interleaved to stdout as S16LE
// ---------------------------------------------------------------------------
void AudioVizApp::pipeOutput() {
    size_t n = std::min(original_.samples.size(), cleaned_.samples.size());
    std::cout.write(reinterpret_cast<const char*>(original_.samples.data()), n * 2);
    std::cout.flush();
}

// ---------------------------------------------------------------------------
// GLFW error callback
// ---------------------------------------------------------------------------
static void glfwErrorCallback(int error, const char* description) {
    std::cerr << "GLFW error " << error << ": " << description << "\n";
}

// ---------------------------------------------------------------------------
// Main run loop
// ---------------------------------------------------------------------------
void AudioVizApp::run() {
    if (pipeMode_) { pipeOutput(); return; }

    glfwSetErrorCallback(glfwErrorCallback);
    if (!glfwInit()) throw std::runtime_error("GLFW init failed");

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(1400, 900, "Audio Cleaner Visualizer", nullptr, nullptr);
    if (!window) { glfwTerminate(); throw std::runtime_error("GLFW window creation failed"); }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        renderUI();

        ImGui::Render();

        int fbW, fbH;
        glfwGetFramebufferSize(window, &fbW, &fbH);
        glViewport(0, 0, fbW, fbH);
        glClearColor(0.12f, 0.12f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
}

// ---------------------------------------------------------------------------
// UI rendering
// ---------------------------------------------------------------------------
// Colours for ImPlotSpec (ImPlot v1.0 API)
static const ImVec4 COL_BEFORE = {0.3f, 0.7f, 1.0f, 1.0f};  // blue  - before
static const ImVec4 COL_AFTER  = {0.2f, 0.9f, 0.4f, 1.0f};  // green - after
static const ImVec4 COL_NOISE  = {1.0f, 0.45f, 0.1f, 1.0f}; // orange - noise filter
static const ImVec4 COL_DIFF   = {1.0f, 0.3f, 0.3f, 1.0f};  // red   - difference
static const ImVec4 COL_CURSOR = {1.0f, 1.0f, 0.0f, 1.0f};  // yellow - playback cursor

// Helper: build an ImPlotSpec with just a line colour
static ImPlotSpec lineSpec(const ImVec4& col, float weight = 1.0f) {
    ImPlotSpec s;
    s.LineColor  = col;
    s.LineWeight = weight;
    return s;
}

void AudioVizApp::renderUI() {
    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos({0, 0});
    ImGui::SetNextWindowSize(io.DisplaySize);
    ImGui::Begin("Audio Cleaner Visualizer", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus);

    // Title bar info
    ImGui::TextColored({0.8f,0.8f,0.8f,1}, "Input: %s   |   Output: %s",
        meta_.input_file.c_str(), meta_.output_file.c_str());
    ImGui::TextColored({0.6f,0.6f,0.6f,1}, "Cmd: %s", meta_.command_line.c_str());
    ImGui::Separator();

    int sr  = original_.sample_rate;
    int ch  = original_.channels;

    // Choose which quiet section to show (tab bar if multiple)
    int sectionIdx = 0;
    if (meta_.quiet_sections.size() > 1) {
        if (ImGui::BeginTabBar("sections")) {
            for (int i = 0; i < static_cast<int>(meta_.quiet_sections.size()); ++i) {
                char label[32];
                snprintf(label, sizeof(label), "Quiet @ %.0fms", meta_.quiet_sections[i].time_ms);
                if (ImGui::BeginTabItem(label)) { sectionIdx = i; ImGui::EndTabItem(); }
            }
            ImGui::EndTabBar();
        }
    }

    float panelW = (io.DisplaySize.x - 24) / 3.0f;
    float panelH = (io.DisplaySize.y - 130) / 2.0f;

    // -----------------------------------------------------------------------
    // Row 1: Noise time | Noise spectrum | Diff spectrum
    // -----------------------------------------------------------------------

    // --- Panel 1: Noise sample time domain ---
    if (ImPlot::BeginPlot("Noise Sample (Time)", {panelW, panelH})) {
        ImPlot::SetupAxes("Time (s)", "Amplitude");
        if (sectionIdx < static_cast<int>(meta_.quiet_sections.size())) {
            const auto& qs = meta_.quiet_sections[sectionIdx];
            size_t n = qs.before_samples.size();
            std::vector<float> tbef(n), vbef(n), taft(n), vaft(n);
            float t0 = qs.time_ms / 1000.0f;
            for (size_t i = 0; i < n; ++i) {
                tbef[i] = taft[i] = t0 + static_cast<float>(i) / sr;
                vbef[i] = qs.before_samples[i] / 32768.0f;
                vaft[i] = qs.after_samples[i]  / 32768.0f;
            }
            ImPlot::PlotLine("Before", tbef.data(), vbef.data(), (int)n, lineSpec(COL_BEFORE));
            ImPlot::PlotLine("After",  taft.data(), vaft.data(), (int)n, lineSpec(COL_AFTER));
        }
        ImPlot::EndPlot();
    }

    ImGui::SameLine();

    // --- Panel 2: Noise spectrum + noise filter overlay ---
    if (ImPlot::BeginPlot("Noise Spectrum", {panelW, panelH})) {
        ImPlot::SetupAxes("Frequency (Hz)", "Magnitude");
        ImPlot::SetupAxisScale(ImAxis_X1, ImPlotScale_Log10);
        if (sectionIdx < static_cast<int>(sectionSpectra_.size())) {
            const auto& ss = sectionSpectra_[sectionIdx];
            int half = static_cast<int>(ss.freqAxis.size());
            ImPlot::PlotLine("Before", ss.freqAxis.data(), ss.beforeMag.data(), half, lineSpec(COL_BEFORE));
            ImPlot::PlotLine("After",  ss.freqAxis.data(), ss.afterMag.data(), half, lineSpec(COL_AFTER));
        }
        if (!avgNoiseSpectrum_.empty() && !freqAxis_.empty()) {
            int half = static_cast<int>(freqAxis_.size());
            ImPlot::PlotLine("Noise Filter", freqAxis_.data(), avgNoiseSpectrum_.data(), half, lineSpec(COL_NOISE, 2.0f));
        }
        ImPlot::EndPlot();
    }

    ImGui::SameLine();

    // --- Panel 3: Difference spectrum ---
    if (ImPlot::BeginPlot("Difference Spectrum", {panelW, panelH})) {
        ImPlot::SetupAxes("Frequency (Hz)", "Magnitude");
        ImPlot::SetupAxisScale(ImAxis_X1, ImPlotScale_Log10);
        if (sectionIdx < static_cast<int>(sectionSpectra_.size())) {
            const auto& ss = sectionSpectra_[sectionIdx];
            int half = static_cast<int>(ss.freqAxis.size());
            ImPlot::PlotLine("Diff", ss.freqAxis.data(), ss.diffMag.data(), half, lineSpec(COL_DIFF));
            if (!avgNoiseSpectrum_.empty()) {
                ImPlot::PlotLine("Noise Filter", freqAxis_.data(), avgNoiseSpectrum_.data(), half, lineSpec(COL_NOISE, 2.0f));
            }
        }
        ImPlot::EndPlot();
    }

    // -----------------------------------------------------------------------
    // Row 2: Before/After time | Before/After spectrum | Diff time
    // -----------------------------------------------------------------------

    // Limit display to 5 seconds for time-domain plots (avoid huge arrays)
    int maxDisplaySamples = sr * ch * 5;

    auto makeDisplaySlice = [&](const std::vector<float>& src) -> std::pair<std::vector<float>,std::vector<float>> {
        int n = std::min((int)src.size(), maxDisplaySamples);
        std::vector<float> t(n), v(n);
        for (int i = 0; i < n; ++i) {
            t[i] = static_cast<float>(i) / (sr * ch);
            v[i] = src[i];
        }
        return {t, v};
    };

    // --- Panel 4: Before/After time domain (first 5 seconds) ---
    if (ImPlot::BeginPlot("Before / After (Time, first 5s)", {panelW, panelH})) {
        ImPlot::SetupAxes("Time (s)", "Amplitude");
        auto [t1, v1] = makeDisplaySlice(origFloat_);
        auto [t2, v2] = makeDisplaySlice(cleanFloat_);
        ImPlot::PlotLine("Before", t1.data(), v1.data(), (int)t1.size(), lineSpec(COL_BEFORE));
        ImPlot::PlotLine("After",  t2.data(), v2.data(), (int)t2.size(), lineSpec(COL_AFTER));
        if (playbackActive_) {
            float cursorT = static_cast<float>(playbackSample_) / (sr * ch);
            double xs[2] = {cursorT, cursorT}, ys[2] = {-1.0, 1.0};
            ImPlot::PlotLine("##cursor", xs, ys, 2, lineSpec(COL_CURSOR, 2.0f));
        }
        ImPlot::EndPlot();
    }

    ImGui::SameLine();

    // --- Panel 5: Before/After spectrum (first 5s averaged) ---
    if (ImPlot::BeginPlot("Before / After (Spectrum)", {panelW, panelH})) {
        ImPlot::SetupAxes("Frequency (Hz)", "Magnitude");
        ImPlot::SetupAxisScale(ImAxis_X1, ImPlotScale_Log10);

        int n = std::min((int)origFloat_.size(), maxDisplaySamples);
        std::vector<float> orig5s(origFloat_.begin(), origFloat_.begin() + n);
        std::vector<float> clean5s(cleanFloat_.begin(), cleanFloat_.begin() + n);
        std::vector<float> origMag, cleanMag;
        computeFFT(orig5s, origMag);
        computeFFT(clean5s, cleanMag);

        size_t half = origMag.size();
        std::vector<float> fa(half);
        for (size_t i = 0; i < half; ++i)
            fa[i] = static_cast<float>(i) * sr / (half * 2 - 2);

        ImPlot::PlotLine("Before", fa.data(), origMag.data(), (int)half, lineSpec(COL_BEFORE));
        ImPlot::PlotLine("After",  fa.data(), cleanMag.data(), (int)half, lineSpec(COL_AFTER));
        if (!avgNoiseSpectrum_.empty() && !freqAxis_.empty()) {
            ImPlot::PlotLine("Noise Filter", freqAxis_.data(), avgNoiseSpectrum_.data(), (int)freqAxis_.size(), lineSpec(COL_NOISE, 2.0f));
        }
        ImPlot::EndPlot();
    }

    ImGui::SameLine();

    // --- Panel 6: Difference time domain ---
    if (ImPlot::BeginPlot("Difference (Time, first 5s)", {panelW, panelH})) {
        ImPlot::SetupAxes("Time (s)", "Amplitude");
        auto [t3, v3] = makeDisplaySlice(diffSamples_);
        ImPlot::PlotLine("Diff", t3.data(), v3.data(), (int)t3.size(), lineSpec(COL_DIFF));
        if (playbackActive_) {
            float cursorT = static_cast<float>(playbackSample_) / (sr * ch);
            double xs[2] = {cursorT, cursorT}, ys[2] = {-1.0, 1.0};
            ImPlot::PlotLine("##cursor", xs, ys, 2, lineSpec(COL_CURSOR, 2.0f));
        }
        ImPlot::EndPlot();
    }

    // -----------------------------------------------------------------------
    // Playback bar
    // -----------------------------------------------------------------------
    renderPlaybackBar();

    ImGui::End();
}

void AudioVizApp::renderPlaybackBar() {
    ImGui::Separator();
    int sr = original_.sample_rate, ch = original_.channels;
    int totalSamples = static_cast<int>(original_.samples.size());
    float totalSecs  = static_cast<float>(totalSamples) / (sr * ch);

    // Play/Pause button
    if (maEngine_) {
        if (ImGui::Button(playbackActive_ ? "⏸  Pause" : "▶  Play")) {
            if (!playbackActive_) {
                // Start or resume playback via miniaudio
                if (!maSound_) {
                    auto* snd = new ma_sound();
                    if (ma_sound_init_from_file(static_cast<ma_engine*>(maEngine_),
                            meta_.output_file.c_str(), 0, nullptr, nullptr, snd) == MA_SUCCESS) {
                        maSound_ = snd;
                        ma_sound_start(snd);
                        playbackActive_ = true;
                    } else {
                        std::cerr << "audio_viz: failed to load sound for playback\n";
                        delete snd;
                    }
                } else {
                    ma_sound_start(static_cast<ma_sound*>(maSound_));
                    playbackActive_ = true;
                }
            } else {
                ma_sound_stop(static_cast<ma_sound*>(maSound_));
                playbackActive_ = false;
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("⏹  Stop")) {
            if (maSound_) {
                ma_sound_stop(static_cast<ma_sound*>(maSound_));
                ma_sound_uninit(static_cast<ma_sound*>(maSound_));
                delete static_cast<ma_sound*>(maSound_);
                maSound_ = nullptr;
            }
            playbackActive_  = false;
            playbackSample_  = 0;
        }

        // Update playback cursor
        if (playbackActive_ && maSound_) {
            ma_uint64 cursor = 0;
            ma_sound_get_cursor_in_pcm_frames(static_cast<ma_sound*>(maSound_), &cursor);
            playbackSample_ = static_cast<int>(cursor) * ch;
            if (!ma_sound_is_playing(static_cast<ma_sound*>(maSound_))) {
                playbackActive_ = false;
                playbackSample_ = 0;
            }
        }
    } else {
        ImGui::TextDisabled("(audio playback unavailable)");
    }

    ImGui::SameLine();
    // Scrubber
    float cursorSecs = static_cast<float>(playbackSample_) / (sr * ch);
    ImGui::SetNextItemWidth(-1);
    if (ImGui::SliderFloat("##scrubber", &cursorSecs, 0.0f, totalSecs, "%.1f s")) {
        playbackSample_ = static_cast<int>(cursorSecs * sr * ch);
        if (maSound_) {
            ma_sound_seek_to_pcm_frame(static_cast<ma_sound*>(maSound_),
                static_cast<ma_uint64>(playbackSample_ / ch));
        }
    }

    ImGui::TextColored({0.5f,0.5f,0.5f,1},
        "Duration: %.1f s  |  %.0f Hz  |  %d ch  |  FFT %d",
        totalSecs, (float)sr, ch, meta_.fft_size);
}
