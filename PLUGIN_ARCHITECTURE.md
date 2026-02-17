# Audio Cleaner Plugin Architecture

## Overview

The Audio Cleaner now features a plugin-based architecture that supports multiple audio formats through a unified interface. This design allows for easy extension with new audio formats without modifying the core processing logic.

## Architecture Components

### Core Interfaces

#### `IAudioReader`
Abstract interface for reading audio files:
- `open(filename)`: Open an audio file for reading
- `read(audioData)`: Read audio data into a vector
- `close()`: Close the file and cleanup resources
- `getFormat()`: Get audio format information
- `getFormatName()`: Get human-readable format name
- `getSupportedExtensions()`: Get list of supported file extensions
- `canHandle(filename)`: Check if this reader can handle the file

#### `IAudioWriter`
Abstract interface for writing audio files:
- `open(filename, format)`: Create/open an audio file for writing
- `write(audioData)`: Write audio data to the file
- `close()`: Close the file and finalize
- `getFormatName()`: Get human-readable format name
- `getSupportedExtensions()`: Get list of supported file extensions
- `canHandle(filename)`: Check if this writer can handle the file

#### `IAudioFormat`
Factory interface for creating readers and writers:
- `createReader()`: Create a new reader instance
- `createWriter()`: Create a new writer instance
- `getFormatName()`: Get format name
- `getSupportedExtensions()`: Get supported extensions
- `canHandle(filename)`: Check format compatibility
- `getPriority()`: Priority for format detection (higher = more priority)

### Factory Pattern

#### `AudioFormatFactory`
Singleton factory that manages all registered audio formats:
- `registerFormat()`: Register a new audio format plugin
- `createReader()`: Create appropriate reader for a file
- `createWriter()`: Create appropriate writer for a file
- `getSupportedFormats()`: List all registered formats
- `isFormatSupported()`: Check if a file extension is supported

### Convenience Layer

#### `AudioLoader`
High-level interface for loading and saving audio:
- `loadAudio()`: Load audio from any supported format
- `saveAudio()`: Save audio to any supported format
- Static methods for format queries

## Built-in Format Plugins

### WAV Plugin (`WavFormat`)
- **Full Implementation**: Complete read/write support
- **Supported Formats**: 16-bit PCM WAV files
- **Extensions**: .wav, .wave
- **Priority**: 10 (high)
- **Features**: Full header validation, proper chunk handling

### MP3 Plugin (`Mp3Format`)
- **Stub Implementation**: Framework ready for real MP3 library integration
- **Extensions**: .mp3
- **Priority**: 8 (medium-high)
- **Status**: Requires external library (mpg123, minimp3, etc.)

### OGG Vorbis Plugin (`OggFormat`)
- **Stub Implementation**: Framework ready for libvorbis integration
- **Extensions**: .ogg, .oga
- **Priority**: 7 (medium)
- **Status**: Requires external library (libvorbis, libogg)

## Adding New Format Plugins

### Step 1: Create Format Classes

```cpp
// include/my_format.h
class MyFormatReader : public IAudioReader {
    // Implement all virtual methods
};

class MyFormatWriter : public IAudioWriter {
    // Implement all virtual methods
};

class MyFormat : public IAudioFormat {
    // Implement factory methods
};
```

### Step 2: Implement the Classes

```cpp
// src/my_format.cpp
#include "../include/my_format.h"

bool MyFormatReader::open(const std::string& filename) {
    // Open file and validate format
    // Set format information
    return true;
}

bool MyFormatReader::read(std::vector<int16_t>& audioData) {
    // Decode and read audio data
    return true;
}

// ... implement other methods
```

### Step 3: Register the Format

```cpp
// In main.cpp or initialization function
void initializeFormats() {
    auto& factory = AudioFormatFactory::getInstance();
    factory.registerFormat(std::make_unique<MyFormat>());
}
```

### Step 4: Update Build System

Add your new source file to CMakeLists.txt:
```cmake
add_executable(audio_cleaner
    # ... existing files
    src/my_format.cpp
)
```

## Real MP3/OGG Integration

### Recommended Libraries

#### MP3 Decoding/Encoding
- **mpg123**: Fast MP3 decoding
- **minimp3**: Header-only MP3 decoder
- **LAME**: MP3 encoding

#### OGG Vorbis Decoding/Encoding
- **libvorbis**: Vorbis codec
- **libogg**: OGG container format
- **libvorbisfile**: High-level Vorbis file API

### Integration Example

```cpp
// Real MP3 implementation using mpg123
#include <mpg123.h>

class RealMp3Reader : public IAudioReader {
private:
    mpg123_handle* handle;
    
public:
    RealMp3Reader() {
        mpg123_init();
        handle = mpg123_new(nullptr, nullptr);
    }
    
    bool open(const std::string& filename) override {
        return mpg123_open(handle, filename.c_str()) == MPG123_OK;
    }
    
    bool read(std::vector<int16_t>& audioData) override {
        // Use mpg123_read to decode audio
        return true;
    }
};
```

### CMake Integration

```cmake
# Add to CMakeLists.txt
find_package(PkgConfig REQUIRED)
pkg_check_modules(MPG123 REQUIRED mpg123)
pkg_check_modules(VORBIS REQUIRED vorbisfile)

target_link_libraries(audio_cleaner 
    ${MPG123_LIBRARIES} 
    ${VORBIS_LIBRARIES}
)
```

## Usage Examples

### List Supported Formats
```bash
audio_cleaner -f
```

### Convert Between Formats
```bash
# MP3 to WAV
audio_cleaner -i input.mp3 -o output.wav

# OGG to WAV with processing
audio_cleaner -i input.ogg -o clean.wav -l 0.005

# WAV to MP3 (if MP3 writing implemented)
audio_cleaner -i input.wav -o output.mp3
```

### Cross-Format Echo Cancellation
```bash
# MP3 input with WAV reference, OGG output
audio_cleaner -i noisy.mp3 -r clean_reference.wav -o clean.ogg
```

## Benefits of Plugin Architecture

1. **Extensibility**: Easy to add new formats without core changes
2. **Modularity**: Each format is self-contained
3. **Testability**: Individual formats can be tested independently
4. **Flexibility**: Runtime format detection and selection
5. **Maintainability**: Clear separation of concerns
6. **Performance**: Format-specific optimizations possible

## Error Handling

The architecture provides comprehensive error handling:
- File format validation
- Graceful fallback for unsupported formats
- Clear error messages for debugging
- Format compatibility checking

## Future Extensions

The plugin architecture enables easy addition of:
- FLAC support
- AAC/M4A support
- Streaming audio support
- Custom audio processing plugins
- Metadata handling plugins
- Real-time processing plugins
