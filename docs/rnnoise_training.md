# RNNoise Custom Model Training Guide

## Overview

RNNoise uses a Gated Recurrent Unit (GRU) neural network to separate speech from
noise in real time. The default pre-trained model works well for general
environments but can be improved for specific noise conditions (e.g. tape hiss,
crowd noise, HVAC, cassette artifacts) by training on domain-specific data.

---

## Hardware Requirements

| Resource | Minimum | Recommended (16 GB GPU) |
|----------|---------|------------------------|
| GPU VRAM | 4 GB    | 16 GB (e.g. RTX 3080 / 4080 / A4000) |
| System RAM | 16 GB | 32 GB |
| Disk | 50 GB | 200 GB (for datasets) |
| Training time | ~48 h (4 GB) | **~6–12 hours** (16 GB) |

A 16 GB GPU is well suited for training RNNoise custom models. The network is
deliberately small (~100 K parameters), so a 16 GB card has ample headroom to use
large batch sizes and significantly reduce training time versus smaller cards.

---

## How Much Training Data Is Required?

### Minimum (acceptable quality)
| Data Type | Duration | Notes |
|-----------|----------|-------|
| Clean speech (foreground) | **10–20 hours** | Studio or near-field recordings, no background noise |
| Background noise (only noise, no speech) | **5–10 hours** | The specific noise type you want suppressed |

### Recommended (good generalisation)
| Data Type | Duration | Notes |
|-----------|----------|-------|
| Clean speech | **50–100 hours** | Diverse speakers, ages, accents, microphones |
| Background noise | **20–40 hours** | Diverse real-world recordings of your target noise type |

### Notes on data quality
- **Speech samples** must be **clean** – no background noise at all. Even faint
  noise in "clean" files degrades model quality significantly.
- **Noise samples** should cover your target acoustic conditions. For old
  cassette tape processing, prioritise: tape hiss, wow/flutter artifacts, vinyl
  crackle, crowd murmur, 50/60 Hz hum, and room reverb.
- Augment noise with random SNR levels (-5 dB to +30 dB) during feature
  extraction – the RNNoise training pipeline does this automatically.

### Recommended Public Datasets
| Dataset | Type | Hours | URL |
|---------|------|-------|-----|
| LibriSpeech train-clean-360 | Speech | 360 h | openslr.org/12 |
| Mozilla Common Voice | Speech | 2800+ h | commonvoice.mozilla.org |
| DEMAND | Noise | 6 h (18 environments) | zenodo.org/record/1227121 |
| FreeSound (ESC-50) | Noise | Varied | freesound.org |
| AudioSet | Mixed | 5800+ h | research.google.com/audioset |

---

## Step-by-Step Training on a 16 GB GPU

### 1. Clone the RNNoise repository

```bash
git clone https://github.com/xiph/rnnoise.git
cd rnnoise
./autogen.sh && ./configure && make -j$(nproc)
```

### 2. Prepare speech and noise directories

```
data/
  speech/        # clean speech WAV files (16-bit, 48kHz mono)
  noise/         # noise-only WAV files (16-bit, 48kHz mono)
```

All files must be **mono, 48 kHz, 16-bit PCM** – RNNoise is fixed at this rate.

```bash
# Convert any file to the required format using ffmpeg
ffmpeg -i input.mp3 -ar 48000 -ac 1 -acodec pcm_s16le output.wav

# Batch convert a directory
for f in speech_raw/*.mp3; do
    ffmpeg -i "$f" -ar 48000 -ac 1 -acodec pcm_s16le \
           "data/speech/$(basename "${f%.mp3}").wav"
done
```

### 3. Extract acoustic features

```bash
# RNNoise provides a feature extraction binary
mkdir -p features

# Extract speech features
for f in data/speech/*.wav; do
    ./dump_features "$f" >> features/speech_features.f32
done

# Extract noise features
for f in data/noise/*.wav; do
    ./dump_features "$f" >> features/noise_features.f32
done
```

### 4. Train the model

RNNoise uses a Python/Keras training script in `src/training/`:

```bash
cd src/training
pip install tensorflow keras numpy scipy

# Train – adjust batch_size for your VRAM
# 16 GB GPU: batch_size=512 works well
python rnn_train.py \
    ../../features/speech_features.f32 \
    ../../features/noise_features.f32 \
    --epochs 120 \
    --batch-size 512 \
    --model-output custom_model.h5
```

**Expected training time on 16 GB GPU:** ~6–12 hours for 120 epochs over 50 h
of combined data. Loss typically converges by epoch 80–100.

### 5. Export the trained weights to C

```bash
# Convert Keras model weights to RNNoise C data file
python dump_rnn.py custom_model.h5 rnnoise_data_custom.c rnnoise_data_custom

# Replace the built-in weights file
cp rnnoise_data_custom.c ../../src/rnnoise_data.c
```

### 6. Rebuild RNNoise with the custom model

```bash
cd /tmp/rnnoise
make -j$(nproc)
sudo make install
sudo ldconfig
```

### 7. Rebuild audio-cleaner

```bash
cd /path/to/audio-cleaner/build
cmake .. -DENABLE_RNNOISE=ON
make -j$(nproc)
```

---

## Usage Modes

### Mode 1 – RNNoise after DSP (recommended)
Spectral subtraction first removes stationary noise, RNNoise then cleans residual:
```bash
./audio_cleaner -i input.mp3 -o output.mp3 --rnnoise
```

### Mode 2 – RNNoise only (fastest)
Skip DSP, use only the neural network:
```bash
./audio_cleaner -i input.mp3 -o output.mp3 --rnnoise-only
```

### Mode 3 – Full pipeline
Combine all algorithms for maximum quality:
```bash
./audio_cleaner -i input.mp3 -o output.mp3 --rnnoise --fft-size 1024 --normalize -3
```

---

## Windows Installation

1. Build RNNoise on WSL2 or cross-compile with MinGW.
2. Copy `rnnoise.dll` (or `rnnoise.lib` + `rnnoise.h`) to `C:\AudioLibs\`.
3. Build with CMake:
   ```powershell
   cmake .. -A x64 `
     -DCMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake `
     -DENABLE_RNNOISE=ON
   cmake --build . --config Release
   ```

---

## GPU Memory vs Batch Size Reference

| GPU VRAM | Recommended batch_size | Epochs to convergence | Approx. time (50h data) |
|----------|----------------------|-----------------------|------------------------|
| 4 GB     | 64                   | 120                   | ~48 hours              |
| 8 GB     | 256                  | 120                   | ~18 hours              |
| **16 GB**| **512**              | **100–120**           | **~6–12 hours**        |
| 24 GB+   | 1024                 | 80–100                | ~4 hours               |

---

## Tips for Cassette/Vintage Audio

- Include **wow and flutter** noise samples (pitch-modulated sine tones).
- Include **tape hiss** (pink/brown noise filtered to match cassette EQ curves).
- Include **vinyl crackle** (impulse noise with specific spectral shape).
- Include **50 Hz / 60 Hz hum** + harmonics.
- Keep clean speech samples from the same era if possible (narrowband
  telephone-quality speech at 300–3400 Hz range responds well).
- Consider pre-processing with `--low-freq 80` to remove subsonic rumble
  before RNNoise to reduce false VAD suppression of low-pitched voices.
