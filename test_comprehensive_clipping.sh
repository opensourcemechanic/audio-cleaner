#!/bin/bash

echo "=== Comprehensive Clipping Test ==="
echo

# Test files
INPUT="rec_20160627-1338_Ciara_1916risingPlay.wav"

echo "1. Analyzing original clipping:"
./tools/simple_clipping_analyzer $INPUT
echo

echo "2. Testing different processing approaches:"
echo

# Test 1: Noise reduction only
echo "   a) Noise reduction only:"
./audio_cleaner -i $INPUT -o test_noise_only.mp3 2>/dev/null | grep "Applied:"
echo

# Test 2: Clipping reduction only (we'll modify the algorithm to skip noise reduction)
echo "   b) Clipping reduction only:"
./audio_cleaner -i $INPUT -o test_clipping_only.mp3 --reduce-clipping 2>/dev/null | grep "Applied:"
echo

# Test 3: Both together
echo "   c) Both noise reduction and clipping reduction:"
./audio_cleaner -i $INPUT -o test_both.mp3 --reduce-clipping 2>/dev/null | grep "Applied:"
echo

# Test 4: Different clipping thresholds
echo "   d) More aggressive clipping reduction (90% threshold):"
# We would need to add a threshold parameter option
echo "      (Would need --clipping-threshold option)"
echo

echo "3. File sizes:"
echo "   Original WAV: $(stat -c%s $INPUT) bytes"
echo "   Noise only:   $(stat -c%s test_noise_only.mp3) bytes"
echo "   Clipping only: $(stat -c%s test_clipping_only.mp3) bytes"
echo "   Both:         $(stat -c%s test_both.mp3) bytes"
echo

echo "4. Recommendations:"
echo "   - Listen to all files around 6:42 and 7:25"
echo "   - Compare harshness of clipping in each version"
echo "   - The clipping reduction should smooth the peaks"
echo "   - If clipping is still harsh, we can make it more aggressive"
echo

echo "=== Test Complete ==="
