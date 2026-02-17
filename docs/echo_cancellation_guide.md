# Echo Cancellation Reference Files Guide

## What is a Reference File?

A reference file contains the **echo/reverb characteristics** of your recording environment. The Audio Cleaner uses this reference to identify and remove echo from your main audio file.

## Types of Reference Files

### 1. **Room Impulse Response** (Best for room echo)
- Captures how sound reflects in your room
- Contains multiple delayed echoes at different volumes
- Ideal for removing room reverb and echo

### 2. **White Noise** (General purpose)
- Low-level random noise
- Good for general echo reduction
- Works when you don't have a specific echo source

### 3. **Real Echo Source** (Most accurate)
- Recording of the actual echo source (TV, speaker, etc.)
- Most effective for targeted echo removal
- Requires recording the echo source separately

## How to Create Reference Files

### Method 1: Use the Built-in Generator (Recommended)

```bash
# Generate room impulse response (10 seconds)
./tools/reference_generator --type impulse --duration 10

# Generate white noise (5 seconds)  
./tools/reference_generator --type noise --duration 5
```

### Method 2: Record Real Room Tone

If you have a microphone:

```bash
# Record 10 seconds of room silence (Linux)
arecord -f cd -d 10 room_reference.wav

# Record 10 seconds of room silence (macOS)
sox -n -r 44100 -c 2 room_reference.wav trim 0 10

# Record 10 seconds of room silence (Windows with Audacity)
# 1. Open Audacity
# 2. Click record
# 3. Record 10 seconds of silence in the echo room
# 4. Export as WAV
```

### Method 3: Record Echo Source

If you know what's causing the echo:

1. **Identify the echo source** (TV, speaker, air conditioner, etc.)
2. **Record just the echo source** without your main audio
3. **Same microphone and position** as your original recording
4. **Same volume levels** as during original recording

## Usage Examples

### Basic Echo Cancellation
```bash
# Use generated room impulse response
./audio_cleaner -i echo_audio.wav -r room_impulse.wav -o clean.wav

# Use generated white noise
./audio_cleaner -i echo_audio.wav -r white_noise.wav -o clean.wav

# Use recorded reference
./audio_cleaner -i echo_audio.wav -r room_reference.wav -o clean.wav
```

### Combined Processing
```bash
# Echo cancellation + low-frequency removal + normalization
./audio_cleaner -i echo_audio.wav -r room_impulse.wav -o clean.wav --low-freq 80 --normalize -6
```

## Reference File Guidelines

### Duration
- **5-10 seconds** is usually sufficient
- Longer files don't improve results significantly
- Must be shorter than main audio file

### Quality
- **44.1kHz sample rate** (same as main audio)
- **16-bit depth** (same as main audio)
- **Stereo format** (same as main audio)
- **No clipping** (keep levels reasonable)

### Content
- **Room impulse**: Multiple delayed reflections
- **White noise**: Low-level random noise (-20dB to -30dB)
- **Real source**: Only the echo source, no main audio

## Troubleshooting

### "Reference signal size mismatch" Warning
This is normal when the reference file is shorter than the main audio. The algorithm will still work.

### No Echo Reduction
- Try a different reference file type
- Ensure reference file matches your recording environment
- Check that echo is actually present in the audio

### Over-Processing
- If audio sounds unnatural, reduce learning rate: `-l 0.005`
- Try noise reduction only: omit `-r` parameter
- Use shorter reference file

## Advanced Usage

### Custom Learning Rate
```bash
# Slower adaptation (more stable)
./audio_cleaner -i audio.wav -r reference.wav -o clean.wav -l 0.005

# Faster adaptation (more aggressive)
./audio_cleaner -i audio.wav -r reference.wav -o clean.wav -l 0.02
```

### Multiple Reference Files
For complex environments, you can try different reference files and compare results:

```bash
# Try room impulse response
./audio_cleaner -i audio.wav -r room_impulse.wav -o clean_impulse.wav

# Try white noise
./audio_cleaner -i audio.wav -r white_noise.wav -o clean_noise.wav

# Compare results and choose the best
```

## Best Practices

1. **Start with generated reference files** - they work well for most cases
2. **Use room impulse response** for room echo/reverb
3. **Use white noise** for general echo reduction
4. **Record real reference** for specific echo sources
5. **Keep reference files short** (5-10 seconds)
6. **Match audio quality** (sample rate, bit depth, channels)
7. **Test different learning rates** if results aren't optimal

## Reference File Types Summary

| Type | Best For | Duration | Creation Method |
|------|----------|----------|-----------------|
| Room Impulse | Room echo, reverb | 5-10s | Generator or recording |
| White Noise | General echo reduction | 5-10s | Generator |
| Real Source | Specific echo sources | 5-10s | Manual recording |

The Audio Cleaner's echo cancellation works by learning the echo characteristics from your reference file and removing those patterns from your main audio. The better the reference matches your echo environment, the better the results!
