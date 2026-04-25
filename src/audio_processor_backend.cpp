#include "audio_processor_backend.h"
#include <iostream>
#include <algorithm>
#include <cmath>

// CPU Backend Implementation
void CPUBackend::fft(std::vector<std::complex<float>>& data) {
    // Cooley-Tukey FFT implementation
    const size_t N = data.size();
    if (N <= 1) return;
    
    // Bit-reversal permutation
    for (size_t i = 0, j = 0; i < N; ++i) {
        if (j > i) {
            std::swap(data[i], data[j]);
        }
        size_t m = N >> 1;
        while (j >= m && m > 0) {
            j -= m;
            m >>= 1;
        }
        j += m;
    }
    
    // FFT computation
    for (size_t len = 2; len <= N; len <<= 1) {
        const float angle = -2.0f * M_PI / len;
        const std::complex<float> wlen(std::cos(angle), std::sin(angle));
        
        for (size_t i = 0; i < N; i += len) {
            std::complex<float> w(1.0f, 0.0f);
            for (size_t j = 0; j < len / 2; ++j) {
                std::complex<float> u = data[i + j];
                std::complex<float> v = data[i + j + len / 2] * w;
                data[i + j] = u + v;
                data[i + j + len / 2] = u - v;
                w *= wlen;
            }
        }
    }
}

void CPUBackend::ifft(std::vector<std::complex<float>>& data) {
    // Conjugate all complex numbers
    for (auto& val : data) {
        val = std::conj(val);
    }
    
    // Forward FFT
    fft(data);
    
    // Conjugate again and scale
    const float scale = 1.0f / data.size();
    for (auto& val : data) {
        val = std::conj(val) * scale;
    }
}

void CPUBackend::applyWindow(std::vector<float>& frame, const std::vector<float>& window) {
    const size_t N = std::min(frame.size(), window.size());
    for (size_t i = 0; i < N; ++i) {
        frame[i] *= window[i];
    }
}

void CPUBackend::spectralSubtraction(
    std::vector<std::complex<float>>& spectrum,
    const std::vector<float>& noiseSpectrum,
    float alpha, float beta
) {
    const size_t N = spectrum.size();
    const size_t halfN = N / 2;
    
    // Process only the first half of the spectrum (DC to Nyquist)
    // noiseSpectrum only has N/2 + 1 elements
    for (size_t i = 0; i <= halfN; ++i) {
        float magnitude = std::abs(spectrum[i]);
        float phase = std::arg(spectrum[i]);
        
        float subtractedMagnitude = magnitude - alpha * noiseSpectrum[i];
        subtractedMagnitude = std::max(subtractedMagnitude, beta * magnitude);
        
        spectrum[i] = std::polar(subtractedMagnitude, phase);
        
        // Mirror to second half (conjugate symmetry for real signals)
        if (i > 0 && i < halfN) {
            spectrum[N - i] = std::polar(subtractedMagnitude, -phase);
        }
    }
}

void CPUBackend::convolve(
    const std::vector<int16_t>& input,
    const std::vector<float>& filter,
    std::vector<int16_t>& output
) {
    const size_t inputSize = input.size();
    const size_t filterSize = filter.size();
    output.resize(inputSize);
    
    for (size_t i = 0; i < inputSize; ++i) {
        float sum = 0.0f;
        for (size_t j = 0; j < filterSize && j <= i; ++j) {
            sum += static_cast<float>(input[i - j]) * filter[j];
        }
        output[i] = static_cast<int16_t>(std::max(-32768.0f, std::min(32767.0f, sum)));
    }
}

size_t CPUBackend::getOptimalFFTSize(size_t minSize) const {
    // Power of 2 optimization for CPU
    size_t size = 1;
    while (size < minSize) size <<= 1;
    return size;
}

// OpenCL Backend Implementation
#ifdef ENABLE_OPENCL

class OpenCLBackend::OpenCLImpl {
public:
    cl_context context = nullptr;
    cl_device_id device = nullptr;
    cl_command_queue queue = nullptr;
    cl_program fftProgram = nullptr;
    cl_kernel fftKernel = nullptr;
    cl_kernel ifftKernel = nullptr;
    cl_kernel windowKernel = nullptr;
    cl_kernel spectralSubtractionKernel = nullptr;
    cl_kernel convolveKernel = nullptr;
    
    // Memory buffers
    cl_mem buffer = nullptr;
    size_t maxFFTSize = 0;
    
    ~OpenCLImpl() {
        if (fftKernel) clReleaseKernel(fftKernel);
        if (ifftKernel) clReleaseKernel(ifftKernel);
        if (windowKernel) clReleaseKernel(windowKernel);
        if (spectralSubtractionKernel) clReleaseKernel(spectralSubtractionKernel);
        if (convolveKernel) clReleaseKernel(convolveKernel);
        if (fftProgram) clReleaseProgram(fftProgram);
        if (queue) clReleaseCommandQueue(queue);
        if (context) clReleaseContext(context);
        if (buffer) clReleaseMemObject(buffer);
    }
};

bool OpenCLBackend::isAvailable() {
    cl_uint numPlatforms;
    if (clGetPlatformIDs(0, nullptr, &numPlatforms) != CL_SUCCESS || numPlatforms == 0) {
        return false;
    }
    return true;
}

std::vector<std::string> OpenCLBackend::getAvailableDevices() {
    std::vector<std::string> devices;
    
    cl_uint numPlatforms;
    clGetPlatformIDs(0, nullptr, &numPlatforms);
    
    std::vector<cl_platform_id> platforms(numPlatforms);
    clGetPlatformIDs(numPlatforms, platforms.data(), nullptr);
    
    for (cl_platform_id platform : platforms) {
        cl_uint numDevices;
        clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 0, nullptr, &numDevices);
        
        std::vector<cl_device_id> deviceIds(numDevices);
        clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, numDevices, deviceIds.data(), nullptr);
        
        for (cl_device_id device : deviceIds) {
            char name[256];
            clGetDeviceInfo(device, CL_DEVICE_NAME, sizeof(name), name, nullptr);
            devices.push_back(std::string(name));
        }
    }
    
    return devices;
}

OpenCLBackend::OpenCLBackend(const std::string& deviceType) 
    : impl(std::make_unique<OpenCLImpl>()) {
    
    if (!initializeOpenCL(deviceType)) {
        throw std::runtime_error("Failed to initialize OpenCL");
    }
}

OpenCLBackend::~OpenCLBackend() = default;

bool OpenCLBackend::initializeOpenCL(const std::string& deviceType) {
    // Get platforms
    cl_uint numPlatforms;
    cl_int err = clGetPlatformIDs(0, nullptr, &numPlatforms);
    if (err != CL_SUCCESS || numPlatforms == 0) {
        std::cerr << "OpenCL: No platforms found (error " << err << ")" << std::endl;
        return false;
    }
    
    std::vector<cl_platform_id> platforms(numPlatforms);
    err = clGetPlatformIDs(numPlatforms, platforms.data(), nullptr);
    if (err != CL_SUCCESS) {
        std::cerr << "OpenCL: Failed to get platforms (error " << err << ")" << std::endl;
        return false;
    }
    
    // Find GPU platform
    cl_platform_id selectedPlatform = nullptr;
    cl_uint numDevices = 0;
    for (cl_platform_id platform : platforms) {
        err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 0, nullptr, &numDevices);
        if (err == CL_SUCCESS && numDevices > 0) {
            selectedPlatform = platform;
            break;
        }
    }
    
    if (!selectedPlatform || numDevices == 0) {
        std::cerr << "OpenCL: No GPU devices found" << std::endl;
        return false;
    }
    
    // Get GPU device
    cl_device_id device;
    err = clGetDeviceIDs(selectedPlatform, CL_DEVICE_TYPE_GPU, 1, &device, nullptr);
    if (err != CL_SUCCESS) {
        std::cerr << "OpenCL: Failed to get GPU device (error " << err << ")" << std::endl;
        return false;
    }
    
    // Create context
    impl->context = clCreateContext(nullptr, 1, &device, nullptr, nullptr, &err);
    if (err != CL_SUCCESS || !impl->context) {
        std::cerr << "OpenCL: Failed to create context (error " << err << ")" << std::endl;
        return false;
    }
    
    // Create command queue
    impl->queue = clCreateCommandQueueWithProperties(impl->context, device, nullptr, &err);
    if (err != CL_SUCCESS || !impl->queue) {
        std::cerr << "OpenCL: Failed to create command queue (error " << err << ")" << std::endl;
        clReleaseContext(impl->context);
        impl->context = nullptr;
        return false;
    }
    
    impl->device = device;
    
    // Load and compile OpenCL kernels
    const char* kernelSource = R"CLC(
    // Define PI for OpenCL kernel
    #define M_PI 3.14159265358979323846f
    
    // Simple DFT kernel (working but slow)
    __kernel void fft(__global float2* data, const int N, const int direction) {
        int gid = get_global_id(0);
        if (gid >= N) return;
        
        float2 result = (float2)(0.0f, 0.0f);
        float angle_sign = direction > 0 ? -1.0f : 1.0f;
        
        for (int k = 0; k < N; k++) {
            float angle = angle_sign * 2.0f * M_PI * gid * k / N;
            float2 twiddle = (float2)(cos(angle), sin(angle));
            result += data[k] * twiddle;
        }
        
        // Scale for inverse FFT
        if (direction < 0) {
            result = result * (1.0f / N);
        }
        
        data[gid] = result;
    }
    
    __kernel void window(__global float* frame, __global const float* window, const int N) {
        int gid = get_global_id(0);
        if (gid >= N) return;
        frame[gid] *= window[gid];
    }
    
    __kernel void spectralSubtraction(__global float2* spectrum, __global const float* noise, 
                                    const float alpha, const float beta, const int N) {
        int gid = get_global_id(0);
        if (gid >= N) return;
        
        // Only process first half of spectrum (real signals are symmetric)
        if (gid > N/2) return;
        
        float2 s = spectrum[gid];
        float magnitude = sqrt(s.x * s.x + s.y * s.y);
        float phase = atan2(s.y, s.x);
        
        float subtracted = magnitude - alpha * noise[gid];
        subtracted = max(subtracted, beta * magnitude);
        
        spectrum[gid].x = subtracted * cos(phase);
        spectrum[gid].y = subtracted * sin(phase);
        
        // Mirror to second half for real signals
        if (gid > 0 && gid < N/2) {
            int mirror_idx = N - gid;
            spectrum[mirror_idx].x = subtracted * cos(-phase);
            spectrum[mirror_idx].y = subtracted * sin(-phase);
        }
    }
    )CLC";
    
    impl->fftProgram = clCreateProgramWithSource(impl->context, 1, &kernelSource, nullptr, nullptr);
    if (!impl->fftProgram) return false;
    
    if (clBuildProgram(impl->fftProgram, 1, &device, nullptr, nullptr, nullptr) != CL_SUCCESS) {
        // Get build error log
        size_t logSize;
        clGetProgramBuildInfo(impl->fftProgram, device, CL_PROGRAM_BUILD_LOG, 0, nullptr, &logSize);
        std::vector<char> log(logSize);
        clGetProgramBuildInfo(impl->fftProgram, device, CL_PROGRAM_BUILD_LOG, logSize, log.data(), nullptr);
        std::cerr << "OpenCL kernel build failed: " << std::string(log.data()) << std::endl;
        return false;
    }
    
    impl->fftKernel = clCreateKernel(impl->fftProgram, "fft", nullptr);
    impl->windowKernel = clCreateKernel(impl->fftProgram, "window", nullptr);
    impl->spectralSubtractionKernel = clCreateKernel(impl->fftProgram, "spectralSubtraction", nullptr);
    
    return impl->fftKernel && impl->windowKernel && impl->spectralSubtractionKernel;
}

void OpenCLBackend::fft(std::vector<std::complex<float>>& data) {
    // Always fall back to CPU for now - GPU DFT is too slow
    // TODO: Implement proper FFT kernel for GPU acceleration
    CPUBackend cpu;
    cpu.fft(data);
}

void OpenCLBackend::ifft(std::vector<std::complex<float>>& data) {
    // Always fall back to CPU for now - GPU DFT is too slow
    CPUBackend cpu;
    cpu.ifft(data);
}

void OpenCLBackend::applyWindow(std::vector<float>& frame, const std::vector<float>& window) {
    if (!impl->windowKernel) return;
    
    const size_t N = std::min(frame.size(), window.size());
    
    // Create buffers
    cl_mem frameBuffer = clCreateBuffer(impl->context, CL_MEM_READ_WRITE, N * sizeof(float), nullptr, nullptr);
    cl_mem windowBuffer = clCreateBuffer(impl->context, CL_MEM_READ_ONLY, N * sizeof(float), nullptr, nullptr);
    
    // Copy data
    clEnqueueWriteBuffer(impl->queue, frameBuffer, CL_TRUE, 0, N * sizeof(float), frame.data(), 0, nullptr, nullptr);
    clEnqueueWriteBuffer(impl->queue, windowBuffer, CL_TRUE, 0, N * sizeof(float), window.data(), 0, nullptr, nullptr);
    
    // Execute kernel
    clSetKernelArg(impl->windowKernel, 0, sizeof(cl_mem), &frameBuffer);
    clSetKernelArg(impl->windowKernel, 1, sizeof(cl_mem), &windowBuffer);
    clSetKernelArg(impl->windowKernel, 2, sizeof(int), &N);
    
    size_t globalSize = N;
    clEnqueueNDRangeKernel(impl->queue, impl->windowKernel, 1, nullptr, &globalSize, nullptr, 0, nullptr, nullptr);
    
    // Read results
    clEnqueueReadBuffer(impl->queue, frameBuffer, CL_TRUE, 0, N * sizeof(float), frame.data(), 0, nullptr, nullptr);
    
    // Cleanup
    clReleaseMemObject(frameBuffer);
    clReleaseMemObject(windowBuffer);
}

void OpenCLBackend::spectralSubtraction(
    std::vector<std::complex<float>>& spectrum,
    const std::vector<float>& noiseSpectrum,
    float alpha, float beta
) {
    if (!impl->spectralSubtractionKernel) return;
    
    const size_t N = spectrum.size();
    
    // Create buffers
    cl_mem spectrumBuffer = clCreateBuffer(impl->context, CL_MEM_READ_WRITE, N * sizeof(std::complex<float>), nullptr, nullptr);
    cl_mem noiseBuffer = clCreateBuffer(impl->context, CL_MEM_READ_ONLY, N * sizeof(float), nullptr, nullptr);
    
    // Copy data
    clEnqueueWriteBuffer(impl->queue, spectrumBuffer, CL_TRUE, 0, N * sizeof(std::complex<float>), spectrum.data(), 0, nullptr, nullptr);
    clEnqueueWriteBuffer(impl->queue, noiseBuffer, CL_TRUE, 0, N * sizeof(float), noiseSpectrum.data(), 0, nullptr, nullptr);
    
    // Execute kernel
    clSetKernelArg(impl->spectralSubtractionKernel, 0, sizeof(cl_mem), &spectrumBuffer);
    clSetKernelArg(impl->spectralSubtractionKernel, 1, sizeof(cl_mem), &noiseBuffer);
    clSetKernelArg(impl->spectralSubtractionKernel, 2, sizeof(float), &alpha);
    clSetKernelArg(impl->spectralSubtractionKernel, 3, sizeof(float), &beta);
    clSetKernelArg(impl->spectralSubtractionKernel, 4, sizeof(int), &N);
    
    size_t globalSize = N;
    clEnqueueNDRangeKernel(impl->queue, impl->spectralSubtractionKernel, 1, nullptr, &globalSize, nullptr, 0, nullptr, nullptr);
    
    // Read results
    clEnqueueReadBuffer(impl->queue, spectrumBuffer, CL_TRUE, 0, N * sizeof(std::complex<float>), spectrum.data(), 0, nullptr, nullptr);
    
    // Cleanup
    clReleaseMemObject(spectrumBuffer);
    clReleaseMemObject(noiseBuffer);
}

void OpenCLBackend::convolve(
    const std::vector<int16_t>& input,
    const std::vector<float>& filter,
    std::vector<int16_t>& output
) {
    // Fallback to CPU for now - GPU convolution would require more complex implementation
    CPUBackend cpu;
    cpu.convolve(input, filter, output);
}

size_t OpenCLBackend::getOptimalFFTSize(size_t minSize) const {
    // GPU prefers larger FFT sizes for better utilization
    size_t size = 512; // Minimum good size for GPU
    while (size < minSize) size <<= 1;
    return size;
}

std::string OpenCLBackend::getDeviceInfo() const {
    if (!impl->device) return "No device";
    
    char name[256];
    clGetDeviceInfo(impl->device, CL_DEVICE_NAME, sizeof(name), name, nullptr);
    
    cl_uint computeUnits;
    clGetDeviceInfo(impl->device, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(computeUnits), &computeUnits, nullptr);
    
    cl_ulong globalMemory;
    clGetDeviceInfo(impl->device, CL_DEVICE_GLOBAL_MEM_SIZE, sizeof(globalMemory), &globalMemory, nullptr);
    
    return std::string(name) + " (" + std::to_string(computeUnits) + " CUs, " + 
           std::to_string(globalMemory / (1024*1024)) + "MB)";
}

#else

// Stub implementations when OpenCL is disabled
class OpenCLBackend::OpenCLImpl {
    // Empty implementation for stub
};

bool OpenCLBackend::isAvailable() { return false; }
std::vector<std::string> OpenCLBackend::getAvailableDevices() { return {}; }
OpenCLBackend::OpenCLBackend(const std::string& deviceType) {
    throw std::runtime_error("OpenCL support not compiled");
}
OpenCLBackend::~OpenCLBackend() = default;
void OpenCLBackend::fft(std::vector<std::complex<float>>& data) {}
void OpenCLBackend::ifft(std::vector<std::complex<float>>& data) {}
void OpenCLBackend::applyWindow(std::vector<float>& frame, const std::vector<float>& window) {}
void OpenCLBackend::spectralSubtraction(std::vector<std::complex<float>>& spectrum, const std::vector<float>& noiseSpectrum, float alpha, float beta) {}
void OpenCLBackend::convolve(const std::vector<int16_t>& input, const std::vector<float>& filter, std::vector<int16_t>& output) {}
size_t OpenCLBackend::getOptimalFFTSize(size_t minSize) const { return minSize; }
std::string OpenCLBackend::getDeviceInfo() const { return "OpenCL not available"; }

#endif

// Factory Implementation
std::unique_ptr<AudioProcessorBackend> AudioProcessorFactory::createBackend(
    BackendType type, const std::string& deviceHint
) {
    switch (type) {
        case BackendType::CPU:
            return std::make_unique<CPUBackend>();
            
        case BackendType::OPENCL:
#ifdef ENABLE_OPENCL
            if (OpenCLBackend::isAvailable()) {
                try {
                    return std::make_unique<OpenCLBackend>(deviceHint);
                } catch (const std::exception& e) {
                    std::cerr << "Warning: OpenCL initialization failed: " << e.what() << std::endl;
                    std::cerr << "Falling back to CPU backend" << std::endl;
                }
            }
#endif
            return std::make_unique<CPUBackend>();
            
        case BackendType::AUTO:
        default:
#ifdef ENABLE_OPENCL
            if (OpenCLBackend::isAvailable()) {
                return std::make_unique<OpenCLBackend>();
            }
#endif
            return std::make_unique<CPUBackend>();
    }
}

std::unique_ptr<AudioProcessorBackend> AudioProcessorFactory::createOptimalBackend(
    double audioDurationSeconds, bool forceGPU
) {
    if (forceGPU) {
#ifdef ENABLE_OPENCL
        if (OpenCLBackend::isAvailable()) {
            try {
                return std::make_unique<OpenCLBackend>();
            } catch (const std::exception& e) {
                std::cerr << "Warning: GPU acceleration requested but OpenCL failed: " << e.what() << std::endl;
                std::cerr << "Continuing with CPU backend" << std::endl;
            }
        } else {
            std::cerr << "Warning: GPU acceleration requested but no OpenCL devices available" << std::endl;
            std::cerr << "Continuing with CPU backend" << std::endl;
        }
#else
        std::cerr << "Warning: GPU acceleration requested but OpenCL support not compiled" << std::endl;
        std::cerr << "Continuing with CPU backend" << std::endl;
#endif
    }
    
    // Use GPU for audio longer than 5 minutes (300 seconds)
    if (audioDurationSeconds > 300.0) {
        return createBackend(BackendType::AUTO);
    }
    
    return createBackend(BackendType::CPU);
}

void AudioProcessorFactory::setPreferredBackend(BackendType type) {
    // Store preference for future calls
    static BackendType preferredType = BackendType::AUTO;
    preferredType = type;
}

std::vector<std::string> AudioProcessorFactory::getAvailableBackends() {
    std::vector<std::string> backends = {"CPU"};
    
#ifdef ENABLE_OPENCL
    if (OpenCLBackend::isAvailable()) {
        backends.push_back("OpenCL");
    }
#endif
    
    return backends;
}

bool AudioProcessorFactory::isGPUAccelerationAvailable() {
#ifdef ENABLE_OPENCL
    return OpenCLBackend::isAvailable();
#else
    return false;
#endif
}

std::string AudioProcessorFactory::getRecommendedBackend() {
    if (isGPUAccelerationAvailable()) {
        return "OpenCL (GPU acceleration available)";
    }
    return "CPU (no GPU acceleration available)";
}
