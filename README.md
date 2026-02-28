# Audio Cleaner

An advanced audio processing application with GPU acceleration support for noise reduction, echo cancellation, and audio restoration.

## Features

- **🔧 Noise Reduction**: Intelligent spectral subtraction with quietest-section noise estimation
- **🎯 Echo Cancellation**: Least Mean Squares (LMS) adaptive filtering
- **📊 Clipping Reduction**: Smooth clipped audio peaks with adjustable thresholds
- **🚀 GPU Acceleration**: OpenCL backend for high-performance processing
- **🎵 Multiple Formats**: Support for WAV, MP3, WMA (with conditional compilation)
- **⚡ Optimized FFT**: 512-point FFT with 75% overlap for fast processing
- **🔍 Cross-Platform**: Windows and Linux compatibility with proper endianness handling

## Performance

- **Fast Processing**: ~9 minutes for 24-minute audio files
- **GPU Acceleration**: OpenCL support for AMD/NVIDIA GPUs
- **Intelligent Algorithms**: Smart noise estimation reduces over-processing
- **Memory Efficient**: Optimized buffer management and streaming

## Quick Start

### Windows

```powershell
# Build the application
.\build_windows.ps1

# Process audio with noise reduction
.\build-windows\audio_cleaner.exe -i input.wav -o clean.wav

# Force GPU acceleration
.\build-windows\audio_cleaner.exe -i input.wav -o clean.wav --force-gpu

# Use specific backend
.\build-windows\audio_cleaner.exe -i input.wav -o clean.wav --backend cpu
```

### Linux

```bash
# Build with CMake
mkdir build && cd build
cmake ..
make

# Process audio
./audio_cleaner -i input.wav -o clean.wav
```

## Usage Examples

```bash
# Basic noise reduction
audio_cleaner -i noisy.wav -o clean.wav

# Echo cancellation with reference
audio_cleaner -i noisy.wav -r reference.wav -o clean.wav

# Reduce clipping in audio
audio_cleaner -i clipped.wav -o smooth.wav --reduce-clipping

# Studio quality processing
audio_cleaner -i input.wav -o output.wav --fft-size 4096

# GPU acceleration for long files
audio_cleaner -i long_audio.wav -o clean.wav --force-gpu
```

## Algorithm Details

### Noise Reduction
- **Intelligent Noise Estimation**: Searches first 30 seconds for quietest frames
- **Spectral Subtraction**: Conservative parameters (ALPHA=0.3, BETA=0.3)
- **Window Function**: Hann window with 75% overlap
- **FFT Processing**: 512-point FFT with O(N log N) algorithm

### Echo Cancellation
- **Adaptive Filtering**: LMS algorithm with configurable learning rate
- **Reference Signal**: Optional reference input for better echo removal
- **Convergence**: Fast convergence with stability guarantees

### Audio Quality
- **Sample Rate**: Supports 8kHz to 192kHz
- **Bit Depth**: 16-bit PCM processing
- **Channels**: Mono and stereo support
- **Formats**: WAV, MP3, WMA (conditional)

## Technical Architecture

### Backend System
- **CPU Backend**: Direct FFT implementation using Cooley-Tukey algorithm
- **OpenCL Backend**: GPU acceleration with fallback to CPU
- **Auto Selection**: Intelligent backend selection based on file size and hardware

### Cross-Platform Compatibility
- **Endianness Handling**: Explicit little-endian WAV file processing
- **Memory Management**: Proper alignment and buffer management
- **Floating-Point**: Consistent numerical behavior across platforms

## Build Requirements

### Windows
- **Compiler**: MinGW-w64 or Visual Studio Build Tools
- **Dependencies**: OpenCL (CUDA Toolkit), optional audio libraries
- **Build Script**: `build_windows.ps1` for automated setup

### Linux
- **Compiler**: GCC or Clang
- **Dependencies**: OpenCL development headers, audio libraries
- **Build System**: CMake

## Configuration

### FFT Size Options
- **128**: Fast processing, lower frequency resolution
- **512**: Default (balanced speed and quality)
- **1024**: Better frequency resolution
- **4096**: Studio quality (slower)

### Backend Selection
- **auto**: Automatic selection based on file size
- **cpu**: Force CPU processing
- **opencl**: Force GPU acceleration

## Troubleshooting

### Windows Audio Distortion
- **Fixed**: Proper endianness handling for WAV files
- **Solution**: Explicit little-endian byte order processing

### GPU Acceleration Issues
- **Fallback**: Automatically falls back to CPU if GPU unavailable
- **Compatibility**: Supports AMD and NVIDIA OpenCL devices

### Over-Processing
- **Solution**: Intelligent noise estimation with conservative parameters
- **Result**: Natural sound with minimal artifacts

## Development

### Code Quality
- **Memory Safety**: Proper bounds checking and buffer management
- **Error Handling**: Comprehensive error reporting and fallbacks
- **Testing**: Extensive testing on various audio formats and platforms

### Performance Optimization
- **FFT Algorithm**: O(N log N) Cooley-Tukey implementation
- **Memory Usage**: Efficient buffer reuse and streaming
- **GPU Utilization**: Optimized OpenCL kernels for audio processing

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Attribution

**Developed by Brian Nitz** with implementation assistance from:
- **Windsurf** IDE with SWE-1.5
- **Claude** (Anthropic) AI assistant for code implementation
- **Development Period**: February 2026

### Development Approach
This project represents a **collaborative development** process:

**Brian Nitz (User) Contributions:**
- Architecture design and performance strategy
- FFT size optimization (512-point default)
- GPU acceleration architecture and backend system
- Performance requirements and debugging direction
- Algorithm selection and optimization goals
- Cross-platform compatibility requirements
- Build system design and testing

**AI Assistant (Claude) Contributions:**
- Code implementation and debugging
- Endianness fix for Windows audio distortion
- Intelligent noise estimation algorithm implementation
- Conservative spectral subtraction parameter tuning
- Memory safety fixes and error handling
- Cross-platform compatibility code implementation

The performance improvements and architectural decisions were **user-driven** based on domain knowledge and requirements, with AI providing technical implementation and problem-solving support.

## Acknowledgments

- **DSP Algorithms**: Based on textbook spectral subtraction and adaptive filtering
- **OpenCL**: GPU acceleration framework
- **Audio Libraries**: Conditional support for various audio codecs
- **Build System**: CMake cross-platform build configuration

---

For more information, bug reports, or feature requests, please refer to the project documentation or create an issue in the repository.
- **Real-time Processing**: Optimized for performance with FFT-based processing
- **Cross-format Processing**: Convert between formats while processing
- **16-bit PCM Support**: Works with standard audio formats
- **Multi-channel Support**: Handles mono and stereo audio

## Supported Formats

### Fully Supported
- **WAV**: 16-bit PCM, mono/stereo, any sample rate

### Framework Ready (requires external libraries)
- **MP3**: Decoding/encoding framework (integrate mpg123/LAME)
- **OGG Vorbis**: Decoding/encoding framework (integrate libvorbis)

### Adding New Formats
The plugin architecture makes it easy to add support for additional formats like FLAC, AAC, etc. See [PLUGIN_ARCHITECTURE.md](PLUGIN_ARCHITECTURE.md) for details.

## Building

### Prerequisites
- CMake 3.10 or higher
- C++17 compatible compiler (GCC, Clang, or MSVC)

### Build Instructions

```bash
# Create build directory
mkdir build
cd build

# Configure and build
cmake ..
cmake --build .

# On Windows with Visual Studio
cmake --build . --config Release
```

### Quick Build Scripts

**Windows (Visual Studio):**
```batch
build.bat
```

**Linux/macOS (GCC/Clang):**
```bash
chmod +x build.sh
./build.sh
```

## Usage

### List Supported Formats
```bash
audio_cleaner -f
```

### Basic Noise Reduction
```bash
audio_cleaner -i noisy.wav -o clean.wav
```

### Cross-format Processing
```bash
# MP3 to WAV
audio_cleaner -i noisy.mp3 -o clean.wav

# OGG to WAV
audio_cleaner -i noisy.ogg -o clean.wav

# WAV to MP3 (if MP3 writing implemented)
audio_cleaner -i input.wav -o output.mp3
```

### Echo Cancellation with Reference Signal
```bash
audio_cleaner -i input.wav -r reference.wav -o output.wav
```

### Advanced Options
```bash
audio_cleaner -i input.wav -r reference.wav -o output.wav -l 0.005
```

### Command Line Options

- `-i <input_file>`: Input audio file (required)
- `-o <output_file>`: Output audio file (required)
- `-r <reference_file>`: Reference signal for echo cancellation (optional)
- `-l <learning_rate>`: Learning rate for adaptive filter (0.001-0.1, default: 0.01)
- `-f`: List supported formats and extensions
- `-h`: Show help message

## Plugin Architecture

The program uses a modular plugin system that allows:

- **Easy Extension**: Add new audio formats without core changes
- **Runtime Detection**: Automatic format identification
- **Flexible I/O**: Read from one format, write to another
- **Modular Testing**: Test formats independently

### Format Detection Priority
1. WAV (Priority: 10) - Full support
2. MP3 (Priority: 8) - Framework ready
3. OGG (Priority: 7) - Framework ready

## Algorithms

### Echo Cancellation
The program uses an adaptive filter with the Least Mean Squares (LMS) algorithm to model and remove echo components. When a reference signal is provided, it estimates the echo path and subtracts the estimated echo from the input signal.

### Noise Reduction
Noise reduction is performed using spectral subtraction:
1. Estimate noise spectrum from the first few frames
2. Transform audio to frequency domain using FFT
3. Subtract estimated noise spectrum
4. Apply spectral flooring to prevent musical noise
5. Transform back to time domain

## Performance

- FFT Size: 1024 samples
- Overlap: 75% (256-sample hop size)
- Window: Hann window
- Processing is optimized for real-time applications

## Example Use Cases

1. **Voice Recording Cleanup**: Remove background noise from voice recordings
2. **Conference Call Enhancement**: Reduce echo and noise in audio conferences
3. **Audio Restoration**: Clean up old or damaged audio recordings
4. **Podcast Production**: Improve audio quality for podcast episodes
5. **Format Conversion**: Convert between formats while cleaning audio

## Adding Real MP3/OGG Support

To enable full MP3 and OGG support, integrate these libraries:

### MP3 Support
```bash
# Ubuntu/Debian
sudo apt-get install libmpg123-dev liblame-dev

# macOS
brew install mpg123 lame
```

### OGG Support
```bash
# Ubuntu/Debian
sudo apt-get install libvorbis-dev libogg-dev

# macOS
brew install libvorbis libogg
```

Then uncomment and configure the CMake options in `CMakeLists.txt`.

## Technical Details

### Dependencies
- Standard C++ library only for WAV support
- Optional external libraries for MP3/OGG
- Math library for FFT operations

### Memory Usage
- Approximately 4x the input audio size during processing
- FFT buffers and filter coefficients add minimal overhead

### CPU Usage
- Optimized for single-threaded performance
- FFT operations are the most computationally intensive
- Suitable for real-time processing on modern CPUs

## File Format Support

- **WAV files**: 16-bit PCM encoding, any sample rate, mono/stereo
- **MP3 files**: Framework ready (requires mpg123/LAME)
- **OGG files**: Framework ready (requires libvorbis)
- **Any duration**: Limited only by available memory

## Limitations

- MP3/OGG support requires external library integration
- Echo cancellation requires a reference signal
- Noise reduction works best for stationary noise
- Processing delay due to FFT-based approach

## Architecture Benefits

- **Extensible**: Easy to add new audio formats
- **Modular**: Each format is self-contained
- **Testable**: Individual formats can be tested independently
- **Flexible**: Runtime format detection and selection
- **Maintainable**: Clear separation of concerns

## License

This project is provided as-is for educational and practical use.

## Contributing

When adding new format support:
1. Create format classes implementing `IAudioReader` and `IAudioWriter`
2. Create a factory class implementing `IAudioFormat`
3. Register the format in the initialization function
4. Update build system with new source files
5. Add tests for the new format

See [PLUGIN_ARCHITECTURE.md](PLUGIN_ARCHITECTURE.md) for detailed implementation guidance.
