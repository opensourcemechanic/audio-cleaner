# RNNoise Installation Guide

RNNoise is a neural network-based noise suppression library developed by Xiph.org. This guide explains how to install it on Windows and Linux to enable advanced noise reduction in the audio-cleaner application.

## What RNNoise Provides

- **Neural network denoising** using GRU (Gated Recurrent Unit) models
- **Real-time processing** with 10ms frame size (480 samples at 48kHz)
- **Voice Activity Detection (VAD)** for intelligent noise gating
- **Wet/dry blend control** to adjust denoising intensity
- **Superior performance** on non-stationary noise compared to traditional DSP

## Installation

### Linux (Ubuntu/Debian)

#### Method 1: Package Manager (Recommended)

```bash
# Install RNNoise development package
sudo apt update
sudo apt install librnnoise-dev

# Build audio-cleaner with RNNoise support
cd audio-cleaner
mkdir build && cd build
cmake .. -DENABLE_RNNOISE=ON
make -j$(nproc)
```

#### Method 2: Build from Source

```bash
# Install dependencies
sudo apt install git build-essential cmake pkg-config

# Clone and build RNNoise
git clone https://github.com/xiph/rnnoise.git
cd rnnoise
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
sudo make install
sudo ldconfig

# Build audio-cleaner
cd ../../audio-cleaner
mkdir build && cd build
cmake .. -DENABLE_RNNOISE=ON
make -j$(nproc)
```

### Windows

#### Method 1: vcpkg (Recommended)

```cmd
# Install vcpkg (if not already installed)
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat

# Install RNNoise
.\vcpkg install rnnoise:x64-windows

# Build audio-cleaner with RNNoise
cd ..\audio-cleaner
mkdir build && cd build
cmake .. -DENABLE_RNNOISE=ON -DCMAKE_TOOLCHAIN_FILE=path\to\vcpkg\scripts\buildsystems\vcpkg.cmake
cmake --build . --config Release
```

#### Method 2: Manual Build

```cmd
# Install Visual Studio with C++ development tools
# Install CMake (https://cmake.org/download/)
# Install Git (https://git-scm.com/download/win)

# Clone and build RNNoise
git clone https://github.com/xiph/rnnoise.git
cd rnnoise
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -A x64
cmake --build . --config Release

# Copy headers and libraries to appropriate location
# (or add to CMAKE_PREFIX_PATH when building audio-cleaner)

# Build audio-cleaner
cd ..\..\audio-cleaner
mkdir build && cd build
cmake .. -DENABLE_RNNOISE=ON -DCMAKE_PREFIX_PATH=path\to\rnnoise\build
cmake --build . --config Release
```

### macOS

```bash
# Install using Homebrew
brew install rnnoise

# Build audio-cleaner
cd audio-cleaner
mkdir build && cd build
cmake .. -DENABLE_RNNOISE=ON
make -j$(nproc)
```

## Verification

After installation, verify RNNoise is working:

```bash
# Check if audio-cleaner was built with RNNoise support
./audio_cleaner --help | grep rnnoise

# Should show:
# --rnnoise           Apply RNNoise neural network denoising (requires librnnoise)
# --rnnoise-only      Apply ONLY RNNoise (skip spectral subtraction DSP)
# --rnnoise-blend <f> Wet/dry mix for RNNoise (0.0=dry, 1.0=full denoise, default: 1.0)
# --rnnoise-vad <f>   Fade toward dry when VAD < threshold (0.0-1.0, default: 0.0=off)

# Test with a noisy audio file
./audio_cleaner -i noisy_file.wav -o cleaned_file.wav --rnnoise
```

## Usage Examples

### Basic Denoising
```bash
./audio_cleaner -i input.wav -o output.wav --rnnoise
```

### Conservative Denoising (preserve more original signal)
```bash
./audio_cleaner -i input.wav -o output.wav --rnnoise --rnnoise-blend 0.5
```

### RNNoise Only (skip spectral subtraction)
```bash
./audio_cleaner -i input.wav -o output.wav --rnnoise-only
```

### With Voice Activity Detection gating
```bash
./audio_cleaner -i input.wav -o output.wav --rnnoise --rnnoise-vad 0.3
```

### Combined with Frequency Filters
```bash
./audio_cleaner -i input.wav -o output.wav --rnnoise --low-freq 80 --high-freq 8000
```

## Troubleshooting

### "RNNoise library not compiled in"
```bash
# Verify RNNoise was found during CMake configuration
cmake .. -DENABLE_RNNOISE=ON 2>&1 | grep RNNoise

# Should show: "RNNoise found - neural network denoising enabled"

# If not found, check library paths
find /usr -name "*rnnoise*" 2>/dev/null
```

### Linker Errors (Linux)
```bash
# Ensure librnnoise-dev is installed
dpkg -l | grep rnnoise

# Check library location
ldconfig -p | grep rnnoise

# Manually specify library path if needed
cmake .. -DENABLE_RNNOISE=ON -Drnnoise_ROOT=/usr/local
```

### Linker Errors (Windows)
```cmd
# Ensure vcpkg toolchain is correctly specified
cmake .. -DENABLE_RNNOISE=ON -DCMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake

# Check if RNNoise was found
cmake .. -DENABLE_RNNOISE=ON --debug-output | findstr rnnoise
```

### Performance Issues

RNNoise requires resampling to 48kHz internally. For very long files:
- Use `--rnnoise-only` to skip spectral subtraction (faster)
- Consider processing in chunks for memory-constrained systems
- CPU usage is higher than spectral subtraction but provides better quality

## Technical Details

- **Sample Rate**: Automatically resamples to 48kHz internally
- **Frame Size**: 480 samples (10ms at 48kHz)
- **Channels**: Processes stereo channels independently with linked VAD
- **Memory**: ~50MB additional memory for neural network models
- **CPU**: Moderate increase in CPU usage vs spectral subtraction

## Comparison: RNNoise vs Spectral Subtraction

| Feature | Spectral Subtraction | RNNoise |
|---------|-------------------|---------|
| **Noise Type** | Best for stationary noise | Excellent for non-stationary noise |
| **Voice Quality** | Good | Excellent (preserves speech) |
| **CPU Usage** | Low | Moderate |
| **Memory Usage** | Low | Moderate (~50MB) |
| **Latency** | Low | Low (10ms frames) |
| **Musical Artifacts** | Possible | Rare |

## Building Without RNNoise

If RNNoise installation fails, you can still build audio-cleaner without it:

```bash
cmake ..  # Without -DENABLE_RNNOISE=ON
make -j$(nproc)
```

The application will still work with spectral subtraction and all other features, but RNNoise options will be disabled.
