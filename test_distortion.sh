#!/bin/bash

# Test script to identify where the distortion issue occurs

echo "=== Audio Cleaner Distortion Investigation ==="
echo

# Test 1: Original file analysis
echo "1. Analyzing original WAV file:"
./tools/wav_reader_test rec_20160627-1338_Ciara_1916risingPlay.wav
echo

# Test 2: Direct FFmpeg conversion (baseline)
echo "2. Direct FFmpeg conversion (baseline):"
ffmpeg -i rec_20160627-1338_Ciara_1916risingPlay.wav -c:a libmp3lame -b:a 128k test_ffmpeg_baseline.mp3 -y 2>/dev/null
echo "✅ FFmpeg direct conversion completed"
echo

# Test 3: Our conversion with processing disabled (if possible)
echo "3. Testing our Audio Cleaner with minimal processing:"
# Let's create a version that just reads and writes without processing
echo "   (This would require a bypass mode in Audio Cleaner)"
echo

# Test 4: Our conversion with current processing
echo "4. Our Audio Cleaner with current processing:"
./audio_cleaner -i rec_20160627-1338_Ciara_1916risingPlay.wav -o test_current_processing.mp3 2>/dev/null
echo "✅ Our processing completed"
echo

# Test 5: Compare file sizes and basic properties
echo "5. File comparison:"
echo "   Original WAV: $(stat -c%s rec_20160627-1338_Ciara_1916risingPlay.wav) bytes"
echo "   FFmpeg MP3:  $(stat -c%s test_ffmpeg_baseline.mp3) bytes"
echo "   Our MP3:     $(stat -c%s test_current_processing.mp3) bytes"
echo

echo "=== Investigation Complete ==="
echo "Listen to the files to determine where distortion occurs:"
echo "- test_ffmpeg_baseline.mp3 (should sound normal)"
echo "- test_current_processing.mp3 (may have distortion)"
echo
echo "If FFmpeg baseline sounds normal but our version doesn't,"
echo "the issue is in our audio processing algorithms."
