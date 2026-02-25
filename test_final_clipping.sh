#!/bin/bash

echo "=== Final Clipping Reduction Comparison ==="
echo

INPUT="rec_20160627-1338_Ciara_1916risingPlay.wav"

echo "Testing different clipping thresholds:"
echo

# Test different thresholds
echo "1. Conservative threshold (95%):"
./audio_cleaner -i $INPUT -o test_95_percent.mp3 --reduce-clipping --clipping-threshold 0.95 2>/dev/null | grep "Clipped samples"
echo

echo "2. Moderate threshold (90%):"
./audio_cleaner -i $INPUT -o test_90_percent.mp3 --reduce-clipping --clipping-threshold 0.90 2>/dev/null | grep "Clipped samples"
echo

echo "3. Aggressive threshold (85%):"
./audio_cleaner -i $INPUT -o test_85_percent.mp3 --reduce-clipping --clipping-threshold 0.85 2>/dev/null | grep "Clipped samples"
echo

echo "4. Very aggressive threshold (80%):"
./audio_cleaner -i $INPUT -o test_80_percent.mp3 --reduce-clipping --clipping-threshold 0.80 2>/dev/null | grep "Clipped samples"
echo

echo "File size comparison:"
echo "  Original WAV: $(stat -c%s $INPUT) bytes"
echo "  95% threshold: $(stat -c%s test_95_percent.mp3) bytes"
echo "  90% threshold: $(stat -c%s test_90_percent.mp3) bytes"
echo "  85% threshold: $(stat -c%s test_85_percent.mp3) bytes"
echo "  80% threshold: $(stat -c%s test_80_percent.mp3) bytes"
echo

echo "=== Recommendations ==="
echo "Listen to the files, especially around 6:42 and 7:25:"
echo "- test_95_percent.mp3: Minimal processing, preserves most dynamics"
echo "- test_90_percent.mp3: Good balance of clipping reduction and preservation"
echo "- test_85_percent.mp3: More aggressive clipping smoothing"
echo "- test_80_percent.mp3: Maximum clipping reduction, may affect dynamics"
echo
echo "The 85-90% range usually provides the best balance for most audio."
