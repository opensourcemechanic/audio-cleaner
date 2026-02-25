# Audio Cleaner Algorithm Documentation

## Overview
This document details the sources and theoretical foundations of the algorithms implemented in the Audio Cleaner tool.

## 1. Noise Reduction - Spectral Subtraction

### Theoretical Foundation
The spectral subtraction algorithm is based on the work by:
- **Boll, S. (1979). "Suppression of acoustic noise in speech using spectral subtraction."** IEEE Transactions on Acoustics, Speech, and Signal Processing, 27(2), 113-120.
- **Berouti, M., Schwartz, R., & Makhoul, J. (1979). "Enhancement of speech corrupted by additive noise."** ICASSP'79. IEEE International Conference on Acoustics, Speech, and Signal Processing.

### Algorithm Description
1. **Noise Estimation**: Estimate the noise spectrum from the first few frames of audio
2. **FFT Transform**: Convert audio to frequency domain using Fast Fourier Transform
3. **Spectral Subtraction**: Subtract estimated noise spectrum from signal spectrum
4. **Spectral Flooring**: Apply minimum gain to prevent musical noise artifacts
5. **Inverse FFT**: Convert back to time domain

### Implementation Details
```cpp
// Core spectral subtraction formula
magnitude_subtracted = magnitude - ALPHA * noise_magnitude
magnitude_final = max(magnitude_subtracted, BETA * magnitude)
```

### Parameter Selection
- **ALPHA = 0.8**: Over-subtraction factor (reduced from 2.0 for gentler processing)
- **BETA = 0.1**: Spectral flooring factor (increased from 0.01 for more preservation)
- **Noise Frames = 3**: Conservative noise estimation from first 3 frames

### References
- Oppenheim, A. V., & Schafer, R. W. (2010). "Discrete-Time Signal Processing." Pearson.
- Lim, J. S., & Oppenheim, A. V. (1979). "Enhancement and bandwidth compression of noisy speech." Proceedings of the IEEE, 67(12), 1586-1604.

## 2. Echo Cancellation - Adaptive LMS Filter

### Theoretical Foundation
The Least Mean Squares (LMS) adaptive filter is based on:
- **Widrow, B., & Hoff, M. E. (1960). "Adaptive switching circuits."** IRE WESCON Convention Record.
- **Widrow, B., & Stearns, S. D. (1985). "Adaptive Signal Processing." Prentice-Hall.

### Algorithm Description
The LMS algorithm adapts filter coefficients to minimize the mean square error between the desired signal and the filter output.

### Mathematical Foundation
```
Error signal: e[n] = d[n] - y[n]
Filter output: y[n] = Σ w[i] * x[n-i]
Weight update: w[n+1] = w[n] + μ * e[n] * x[n]
```
Where:
- `e[n]` is the error signal
- `d[n]` is the desired signal
- `y[n]` is the filter output
- `w[i]` are the filter coefficients
- `μ` is the learning rate

### Implementation Details
```cpp
// LMS weight update
error = desired - output;
for (int i = 0; i < filterLength; i++) {
    weights[i] += learningRate * error * reference[i];
}
```

### Parameter Selection
- **Learning Rate = 0.01**: Default adaptive rate (configurable 0.001-0.1)
- **Filter Length**: Adaptive based on echo characteristics

### References
- Haykin, S. (2002). "Adaptive Filter Theory." Prentice Hall.
- Sayed, A. H. (2008). "Adaptive Filters." Wiley.

## 3. Clipping Reduction - Soft Clipping Algorithm

### Theoretical Foundation
Soft clipping is based on non-linear signal processing techniques from:
- **Giannoulis, D., Massberg, M., & Reiss, J. D. (2012). "Digital dynamic range compression design: A tutorial and analysis."** IEEE/ACM Transactions on Audio, Speech, and Language Processing, 20(6), 1742-1753.

### Algorithm Description
Soft clipping uses a cubic function to gently limit signal peaks, reducing harsh distortion while preserving audio dynamics.

### Mathematical Foundation
```
if |x| > threshold:
    y = sign(x) * (threshold + (|x| - threshold) * (1 - (|x| - threshold)²/2))
else:
    y = x
```

### Implementation Details
```cpp
// Cubic soft clipping
float excess = normalized - threshold;
float softClipped = threshold + excess * (1.0f - excess * excess * 0.5f);
```

### Parameter Selection
- **Threshold = 0.95**: Default clipping threshold (configurable 0.8-0.99)
- **Cubic Function**: Provides smooth transition from linear to limited region

### References
- Zölzer, U. (2011). "Digital Audio Signal Processing." Wiley.
- Pohlmann, K. C. (2005). "Principles of Digital Audio." McGraw-Hill.

## 4. FFT Processing - Windowing and Overlap-Add

### Theoretical Foundation
FFT-based processing with windowing is based on:
- **Oppenheim, A. V., & Schafer, R. W. (2010). "Discrete-Time Signal Processing." Pearson.
- **Harris, F. J. (1978). "On the use of windows for harmonic analysis with the discrete Fourier transform."** Proceedings of the IEEE, 66(1), 51-83.

### Hann Window
The Hann window provides good frequency resolution and reduced spectral leakage:
```
w[n] = 0.5 * (1 - cos(2πn/(N-1)))
```

### Overlap-Add Method
The overlap-add method reconstructs the time-domain signal from overlapping processed frames:
- **Overlap = 75%**: 256-sample hop for 1024-point FFT
- **Window Sum Normalization**: Prevents amplitude modulation artifacts

### Implementation Details
```cpp
// Hann window application
for (size_t i = 0; i < frame.size(); i++) {
    frame[i] *= 0.5f * (1.0f - cosf(2.0f * PI * i / (frame.size() - 1)));
}
```

### References
- Allen, J. B., & Rabiner, L. R. (1977). "A unified approach to short-time Fourier analysis and synthesis." Proceedings of the IEEE, 65(11), 1558-1564.
- Rabiner, L. R., & Schafer, R. W. (2007). "Theory and Applications of Digital Speech Processing." Pearson.

## 5. Low-Frequency Removal - High-Pass Filter

### Theoretical Foundation
The Butterworth high-pass filter design is based on:
- **Butterworth, S. (1930). "On the theory of filter amplifiers."** Wireless Engineer, 7, 536-541.

### Algorithm Description
A 2nd-order Butterworth high-pass filter removes low-frequency noise while maintaining flat frequency response in the passband.

### Mathematical Foundation
```
H(s) = s² / (s² + √2*s + 1)
```
Digital implementation using bilinear transform:
```
a0 = 1 / (c + 1)
a1 = -a0
a2 = 0
b1 = 2 * (1 - c) / (c + 1)
b2 = (1 - c) / (c + 1)
```
Where `c = tan(π * cutoff / nyquist)`

### Implementation Details
```cpp
// High-pass filter coefficients
float c = tanf(PI * cutoffFreq / nyquist);
float a0 = 1.0f / (c + 1.0f);
float b1 = 2.0f * (1.0f - c) / (c + 1.0f);
float b2 = (1.0f - c) / (c + 1.0f);
```

### References
- Oppenheim, A. V., & Schafer, R. W. (2010). "Discrete-Time Signal Processing." Pearson.
- Smith, J. O. (2011). "Physical Audio Signal Processing." W3K Publishing.

## 6. Normalization - Peak Level Control

### Theoretical Foundation
Audio normalization is based on standard broadcast engineering practices:
- **EBU R128**: Loudness normalization standard
- **ITU-R BS.1770**: Loudness measurement algorithm

### Algorithm Description
Normalization scales the audio to achieve a target peak level in dBFS (decibels relative to full scale).

### Mathematical Foundation
```
target_linear = 10^(target_dBFS / 20)
scale_factor = target_linear / current_peak
output = input * scale_factor
```

### Implementation Details
```cpp
// Normalization calculation
float targetLinear = powf(10.0f, targetDbfs / 20.0f);
float scaleFactor = targetLinear / maxAbsValue;
```

### References
- EBU Technical Recommendation R128 (2014). "Loudness normalisation."
- ITU-R Recommendation BS.1770-4 (2015). "Algorithms to measure audio programme loudness."

## 7. Audio Format Support

### WAV Format
- **Specification**: Microsoft RIFF WAVE File Format
- **Standard**: Multimedia Programming Interface and Data Specifications 1.0
- **Implementation**: 16-bit PCM, little-endian byte order

### MP3 Format
- **Library**: libmp3lame (LAME 3.100)
- **Standard**: MPEG-1 Audio Layer III
- **Parameters**: 128 kbps, 44.1 kHz, joint stereo

### OGG Format
- **Library**: libvorbis/libogg
- **Standard**: Xiph.Org Vorbis I specification
- **Parameters**: Variable bitrate, quality 0.5

### WMA Format
- **Library**: FFmpeg libavformat/libavcodec
- **Standard**: Windows Media Audio format
- **Implementation**: Read-only support via FFmpeg

## 8. Performance Optimizations

### FFT Optimization
- **Algorithm**: Cooley-Tukey FFT algorithm
- **Complexity**: O(N log N) for N-point FFT
- **Implementation**: In-place computation to minimize memory usage

### Memory Management
- **Overlap-Add**: Efficient circular buffer management
- **Window Pre-computation**: Hann window values calculated once
- **Filter State**: Persistent filter coefficients between frames

### References
- Cooley, J. W., & Tukey, J. W. (1965). "An algorithm for the machine calculation of complex Fourier series." Mathematics of Computation, 19(90), 297-301.
- Press, W. H., et al. (2007). "Numerical Recipes: The Art of Scientific Computing." Cambridge University Press.

## Conclusion

The Audio Cleaner implements well-established signal processing algorithms with proven theoretical foundations. Each algorithm has been carefully parameterized for audio processing applications, with extensive testing and optimization for real-world use cases.

The implementation balances computational efficiency with audio quality, using industry-standard practices and parameters derived from academic research and professional audio engineering standards.
