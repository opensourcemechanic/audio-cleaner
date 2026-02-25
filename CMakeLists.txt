cmake_minimum_required(VERSION 3.10)
project(AudioCleaner VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Optional OpenCL GPU acceleration
option(ENABLE_OPENCL "Enable OpenCL GPU acceleration" OFF)

# Try to detect and set compiler
if(MSVC)
    set(CMAKE_CXX_FLAGS "/W4 /O2")
else()
    set(CMAKE_CXX_FLAGS "-Wall -Wextra -O3")
endif()

include_directories(include)

add_executable(audio_cleaner
    src/main.cpp
    src/audio_loader.cpp
    src/audio_processor.cpp
    src/wav_format.cpp
    src/mp3_format.cpp
    src/ogg_format.cpp
    src/wma_format.cpp
    src/audio_processor_backend.cpp
)

# Link math library on non-Windows systems
if(NOT MSVC)
    target_link_libraries(audio_cleaner m)
endif()

# Add OpenCL support if enabled
if(ENABLE_OPENCL)
    find_package(OpenCL REQUIRED)
    if(OpenCL_FOUND)
        target_compile_definitions(audio_cleaner PRIVATE ENABLE_OPENCL)
        target_link_libraries(audio_cleaner ${OpenCL_LIBRARIES})
        target_include_directories(audio_cleaner PRIVATE ${OpenCL_INCLUDE_DIRS})
        message(STATUS "OpenCL GPU acceleration enabled")
    else()
        message(WARNING "OpenCL requested but not found - building without GPU support")
    endif()
endif()

# Add external library support for real MP3/OGG decoding
find_package(PkgConfig REQUIRED)
pkg_check_modules(MPG123 REQUIRED libmpg123)
pkg_check_modules(VORBIS REQUIRED vorbisfile)
find_library(LAME_LIB REQUIRED NAMES mp3lame)

# Add FFmpeg support for WMA
find_path(AVFORMAT_INCLUDE_DIR libavformat/avformat.h)
find_path(AVCODEC_INCLUDE_DIR libavcodec/avcodec.h)
find_path(AVUTIL_INCLUDE_DIR libavutil/avutil.h)
find_path(SWRESAMPLE_INCLUDE_DIR libswresample/swresample.h)

find_library(AVFORMAT_LIB avformat)
find_library(AVCODEC_LIB avcodec) 
find_library(AVUTIL_LIB avutil)
find_library(SWRESAMPLE_LIB swresample)

target_link_libraries(audio_cleaner ${MPG123_LIBRARIES} ${VORBIS_LIBRARIES} ${LAME_LIB} 
                      ${AVFORMAT_LIB} ${AVCODEC_LIB} ${AVUTIL_LIB} ${SWRESAMPLE_LIB})

install(TARGETS audio_cleaner
    RUNTIME DESTINATION bin
)
