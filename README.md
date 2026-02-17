# Audio Cleaner

A fast C++ program with plugin architecture for processing audio files with echo cancellation and background noise reduction capabilities.

## Features

- **Plugin Architecture**: Extensible format support through plugin system
- **Echo Cancellation**: Uses adaptive filtering (LMS algorithm) to remove echoes
- **Noise Reduction**: Implements spectral subtraction for background noise removal
- **Multi-format Support**: WAV (full), MP3/OGG (framework ready)
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
