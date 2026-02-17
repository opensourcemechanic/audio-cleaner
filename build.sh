#!/bin/bash
echo "Building Audio Cleaner with Plugin Architecture..."

# Compile with g++ and all format plugins
g++ -std=c++17 -Wall -Wextra -O3 -Iinclude \
    src/main.cpp \
    src/wav_format.cpp \
    src/mp3_format.cpp \
    src/ogg_format.cpp \
    src/audio_loader.cpp \
    src/audio_processor.cpp \
    -o audio_cleaner -lm

if [ $? -eq 0 ]; then
    echo "Build successful!"
    echo "Executable: audio_cleaner"
    chmod +x audio_cleaner
    echo ""
    echo "Plugin architecture loaded with support for:"
    echo "  - WAV (full support)"
    echo "  - MP3 (framework - requires external library)"
    echo "  - OGG (framework - requires external library)"
else
    echo "Build failed!"
    exit 1
fi
