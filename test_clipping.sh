#!/bin/bash

# Test clipping reduction feature

echo "=== Clipping Reduction Test ==="
echo

# Test 1: Original file with noise reduction only
echo "1. Testing with noise reduction only:"
./audio_cleaner -i rec_20160627-1338_Ciara_1916risingPlay.wav -o test_noise_only.mp3 2>/dev/null | tail -10
echo

# Test 2: Original file with clipping reduction only (bypass noise reduction)
echo "2. Testing with clipping reduction only:"
# We need to create a version that bypasses noise reduction for this test
echo "   (Would need --bypass-noise-reduction option)"
echo

# Test 3: Original file with both noise reduction and clipping reduction
echo "3. Testing with both noise reduction and clipping reduction:"
./audio_cleaner -i rec_20160627-1338_Ciara_1916risingPlay.wav -o test_both.mp3 --reduce-clipping 2>/dev/null | tail -10
echo

# Test 4: Compare file sizes
echo "4. File comparison:"
echo "   Original WAV: $(stat -c%s rec_20160627-1338_Ciara_1916risingPlay.wav) bytes"
echo "   Noise only:   $(stat -c%s test_noise_only.mp3) bytes"
echo "   Both:         $(stat -c%s test_both.mp3) bytes"
echo

echo "=== Test Complete ==="
echo "Listen to the files around 6:42 to compare clipping reduction:"
echo "- test_noise_only.mp3 (baseline)"
echo "- test_both.mp3 (with clipping reduction)"
echo
echo "The clipping reduction should smooth the harsh peaks around 6:42"
