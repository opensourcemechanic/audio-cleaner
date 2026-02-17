# Example Usage

## Building the Program

### Windows (Visual Studio)
```batch
# Run the build script
build.bat

# Or manually with Visual Studio Developer Command Prompt:
cl /EHsc /W4 /O2 /Iinclude src\main.cpp src\wav_reader.cpp src\audio_processor.cpp /Fe:audio_cleaner.exe
```

### Linux/macOS (GCC/Clang)
```bash
# Make build script executable and run
chmod +x build.sh
./build.sh

# Or manually:
g++ -std=c++17 -Wall -Wextra -O3 -Iinclude src/main.cpp src/wav_reader.cpp src/audio_processor.cpp -o audio_cleaner -lm
```

### Using CMake
```bash
mkdir build && cd build
cmake ..
cmake --build .
```

## Running the Program

### Basic Noise Reduction
```bash
# Remove background noise from a recording
audio_cleaner -i noisy_recording.wav -o clean_recording.wav
```

### Echo Cancellation
```bash
# Remove echo using a reference signal (e.g., the speaker output)
audio_cleaner -i microphone_input.wav -r speaker_output.wav -o clean_output.wav
```

### Advanced Processing
```bash
# Custom learning rate for better echo cancellation
audio_cleaner -i input.wav -r reference.wav -o output.wav -l 0.005
```

## Typical Use Cases

### 1. Voice Recording Cleanup
```bash
# Clean up a voice recording with background noise
audio_cleaner -i voice_with_noise.wav -o clean_voice.wav
```

### 2. Conference Call Enhancement
```bash
# If you have the original speaker output as reference
audio_cleaner -i microphone.wav -r speakers.wav -o enhanced_audio.wav
```

### 3. Audio Restoration
```bash
# Restore old recordings (noise reduction only)
audio_cleaner -i old_recording.wav -o restored_recording.wav
```

## Parameters Explained

- **Learning Rate (-l)**: Controls how quickly the adaptive filter learns
  - Lower values (0.001): Slower but more stable
  - Higher values (0.1): Faster but potentially less stable
  - Default (0.01): Good balance for most cases

## Tips for Best Results

1. **For Echo Cancellation**: 
   - Use a clean reference signal recorded from the speakers
   - Ensure reference and input have the same sample rate
   - Start with default learning rate, adjust if needed

2. **For Noise Reduction**:
   - Works best with stationary noise (hiss, fan noise, etc.)
   - May need multiple passes for very noisy audio
   - Some musical artifacts may appear with aggressive settings

3. **File Format**:
   - Only 16-bit PCM WAV files are supported
   - Convert other formats to WAV first
   - Use same sample rate for reference and input files

## Troubleshooting

### "Invalid WAV file format"
- Ensure the file is 16-bit PCM WAV
- Try converting with Audacity or ffmpeg

### "Reference signal size mismatch"
- Make sure reference and input files have the same duration
- Check that sample rates match

### Poor echo cancellation
- Try adjusting the learning rate
- Ensure reference signal is clean and synchronized
- Check that reference actually contains the echo source

### Musical noise after processing
- This is normal with spectral subtraction
- Try reducing the aggressiveness (would need code modification)
- Use higher quality input recordings
