#pragma once
#include <vector>
#include <complex>
#include <memory>
#include <string>

// Forward declarations for OpenCL types
#ifdef ENABLE_OPENCL
#include <CL/cl.h>
#endif

// Abstract interface for audio processing backends
class AudioProcessorBackend {
public:
    virtual ~AudioProcessorBackend() = default;
    
    // Core FFT operations
    virtual void fft(std::vector<std::complex<float>>& data) = 0;
    virtual void ifft(std::vector<std::complex<float>>& data) = 0;
    
    // Windowing operations
    virtual void applyWindow(std::vector<float>& frame, const std::vector<float>& window) = 0;
    
    // Spectral processing
    virtual void spectralSubtraction(
        std::vector<std::complex<float>>& spectrum,
        const std::vector<float>& noiseSpectrum,
        float alpha, float beta
    ) = 0;
    
    // Convolution for echo cancellation
    virtual void convolve(
        const std::vector<int16_t>& input,
        const std::vector<float>& filter,
        std::vector<int16_t>& output
    ) = 0;
    
    // Backend capabilities
    virtual bool isGPUAccelerated() const = 0;
    virtual std::string getBackendName() const = 0;
    virtual size_t getOptimalFFTSize(size_t minSize) const = 0;
    virtual std::string getDeviceInfo() const = 0;
};

// CPU implementation (current code)
class CPUBackend : public AudioProcessorBackend {
public:
    void fft(std::vector<std::complex<float>>& data) override;
    void ifft(std::vector<std::complex<float>>& data) override;
    void applyWindow(std::vector<float>& frame, const std::vector<float>& window) override;
    void spectralSubtraction(
        std::vector<std::complex<float>>& spectrum,
        const std::vector<float>& noiseSpectrum,
        float alpha, float beta
    ) override;
    void convolve(
        const std::vector<int16_t>& input,
        const std::vector<float>& filter,
        std::vector<int16_t>& output
    ) override;
    
    bool isGPUAccelerated() const override { return false; }
    std::string getBackendName() const override { return "CPU"; }
    size_t getOptimalFFTSize(size_t minSize) const override;
    std::string getDeviceInfo() const override { return "Scalar CPU processing"; }
};

// OpenCL GPU implementation (optional, completely separate)
class OpenCLBackend : public AudioProcessorBackend {
public:
    static bool isAvailable();
    static std::vector<std::string> getAvailableDevices();
    
    OpenCLBackend(const std::string& deviceType = "auto");
    ~OpenCLBackend();
    
    void fft(std::vector<std::complex<float>>& data) override;
    void ifft(std::vector<std::complex<float>>& data) override;
    void applyWindow(std::vector<float>& frame, const std::vector<float>& window) override;
    void spectralSubtraction(
        std::vector<std::complex<float>>& spectrum,
        const std::vector<float>& noiseSpectrum,
        float alpha, float beta
    ) override;
    void convolve(
        const std::vector<int16_t>& input,
        const std::vector<float>& filter,
        std::vector<int16_t>& output
    ) override;
    
    bool isGPUAccelerated() const override { return true; }
    std::string getBackendName() const override { return "OpenCL"; }
    size_t getOptimalFFTSize(size_t minSize) const override;
    std::string getDeviceInfo() const override;
    
private:
    class OpenCLImpl;
    std::unique_ptr<OpenCLImpl> impl;
    
    // Initialize OpenCL context and kernels
    bool initializeOpenCL(const std::string& deviceType);
    void cleanupOpenCL();
};

// Factory for creating optimal backend
class AudioProcessorFactory {
public:
    enum class BackendType {
        AUTO,    // Choose best available
        CPU,     // Force CPU
        OPENCL   // Force OpenCL if available
    };
    
    static std::unique_ptr<AudioProcessorBackend> createBackend(
        BackendType type = BackendType::AUTO,
        const std::string& deviceHint = ""
    );
    
    static void setPreferredBackend(BackendType type);
    static std::vector<std::string> getAvailableBackends();
    static bool isGPUAccelerationAvailable();
    static std::string getRecommendedBackend();
    
    // Smart backend selection based on audio duration
    static std::unique_ptr<AudioProcessorBackend> createOptimalBackend(
        double audioDurationSeconds,
        bool forceGPU = false
    );
};
