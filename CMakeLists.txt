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

# Always include core files
set(CORE_SOURCES
    src/main.cpp
    src/audio_loader.cpp
    src/audio_processor.cpp
    src/wav_format.cpp
    src/audio_processor_backend.cpp
)

# Add optional format sources based on library availability
set(OPTIONAL_SOURCES "")

if(NOT WIN32)
    # Linux - assume libraries are available
    list(APPEND OPTIONAL_SOURCES src/mp3_format.cpp src/ogg_format.cpp src/wma_format.cpp)
endif()

add_executable(audio_cleaner ${CORE_SOURCES} ${OPTIONAL_SOURCES})

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
if(WIN32)
    # Windows: Use find_library instead of pkg-config
    # Add custom path for manual installation
    set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH};C:/AudioLibs")
    
    find_library(MPG123_LIB NAMES mpg123-0 mpg123 PATHS "C:/AudioLibs/lib")
    find_library(VORBIS_LIB NAMES vorbisfile PATHS "C:/AudioLibs/lib")
    find_library(VORBIS_ENC_LIB NAMES vorbisenc PATHS "C:/AudioLibs/lib")
    find_library(OGG_LIB NAMES ogg PATHS "C:/AudioLibs/lib")
    find_library(LAME_LIB NAMES mp3lame PATHS "C:/AudioLibs/lib")
    
    # Also check for headers
    find_path(MPG123_INCLUDE_DIR mpg123.h PATHS "C:/AudioLibs/include")
    find_path(LAME_INCLUDE_DIR lame/lame.h PATHS "C:/AudioLibs/include")
    
    if(NOT MPG123_LIB OR NOT VORBIS_LIB OR NOT LAME_LIB)
        message(WARNING "Audio libraries not found - some formats may not be supported")
        message(STATUS "To install audio libraries, place them in C:/AudioLibs/")
    endif()
else()
    # Linux: Use pkg-config
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(MPG123 REQUIRED libmpg123)
    pkg_check_modules(VORBIS REQUIRED vorbisfile)
    find_library(LAME_LIB REQUIRED NAMES mp3lame)
endif()

# Add FFmpeg support for WMA
find_path(AVFORMAT_INCLUDE_DIR libavformat/avformat.h)
find_path(AVCODEC_INCLUDE_DIR libavcodec/avcodec.h)
find_path(AVUTIL_INCLUDE_DIR libavutil/avutil.h)
find_path(SWRESAMPLE_INCLUDE_DIR libswresample/swresample.h)

find_library(AVFORMAT_LIB avformat)
find_library(AVCODEC_LIB avcodec) 
find_library(AVUTIL_LIB avutil)
find_library(SWRESAMPLE_LIB swresample)

# Link libraries that were found
set(LINKED_LIBS "")

# Add audio libraries if found and define macros
if(WIN32)
    if(MPG123_LIB AND MPG123_INCLUDE_DIR AND LAME_LIB AND LAME_INCLUDE_DIR)
        target_sources(audio_cleaner PRIVATE src/mp3_format.cpp)
        list(APPEND LINKED_LIBS ${MPG123_LIB} ${LAME_LIB})
        target_compile_definitions(audio_cleaner PRIVATE HAVE_MPG123 HAVE_LAME)
        target_include_directories(audio_cleaner PRIVATE ${MPG123_INCLUDE_DIR} ${LAME_INCLUDE_DIR})
    endif()
    if(VORBIS_LIB AND OGG_LIB)
        target_sources(audio_cleaner PRIVATE src/ogg_format.cpp)
        list(APPEND LINKED_LIBS ${VORBIS_LIB} ${OGG_LIB})
        target_compile_definitions(audio_cleaner PRIVATE HAVE_VORBIS HAVE_OGG)
    endif()
    if(AVFORMAT_LIB AND AVCODEC_LIB AND AVUTIL_LIB AND SWRESAMPLE_LIB)
        target_sources(audio_cleaner PRIVATE src/wma_format.cpp)
    endif()
else()
    list(APPEND LINKED_LIBS ${MPG123_LIBRARIES} ${VORBIS_LIBRARIES} ${LAME_LIB})
    target_compile_definitions(audio_cleaner PRIVATE HAVE_MPG123 HAVE_VORBIS HAVE_OGG HAVE_LAME)
endif()

# Add FFmpeg libraries if found and define macro
set(FFMPEG_FOUND FALSE)
if(AVFORMAT_LIB AND AVCODEC_LIB AND AVUTIL_LIB AND SWRESAMPLE_LIB)
    list(APPEND LINKED_LIBS ${AVFORMAT_LIB} ${AVCODEC_LIB} ${AVUTIL_LIB} ${SWRESAMPLE_LIB})
    target_compile_definitions(audio_cleaner PRIVATE HAVE_FFMPEG)
    set(FFMPEG_FOUND TRUE)
    message(STATUS "FFmpeg libraries found - WMA support enabled")
else()
    message(STATUS "FFmpeg libraries not found - WMA support disabled")
endif()

# Link all found libraries
if(LINKED_LIBS)
    target_link_libraries(audio_cleaner ${LINKED_LIBS})
endif()

# Copy required DLLs to build directory on Windows
if(WIN32)
    if(MPG123_LIB AND LAME_LIB)
        add_custom_command(TARGET audio_cleaner POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            C:/AudioLibs/lib/mpg123.dll
            C:/AudioLibs/lib/libmp3lame.dll
            $<TARGET_FILE_DIR:audio_cleaner>)
    endif()
endif()

install(TARGETS audio_cleaner
    RUNTIME DESTINATION bin
)

# =============================================================================
# Optional: audio_viz visualizer tool
# Build with: cmake -DBUILD_VISUALIZER=ON ..
# Requires: GLFW3, OpenGL, and internet access for imgui/implot (fetched via git)
# =============================================================================
option(BUILD_VISUALIZER "Build audio_viz visualization tool" OFF)

if(BUILD_VISUALIZER)
    find_package(OpenGL REQUIRED)
    find_package(glfw3 REQUIRED)

    set(IMGUI_DIR  ${CMAKE_SOURCE_DIR}/tools/imgui_src)
    set(IMPLOT_DIR ${CMAKE_SOURCE_DIR}/tools/implot_src)

    # Check that ImGui/ImPlot sources were cloned
    if(NOT EXISTS "${IMGUI_DIR}/imgui.h")
        message(FATAL_ERROR
            "ImGui sources not found at ${IMGUI_DIR}.\n"
            "Run: cd tools && git clone --depth=1 https://github.com/ocornut/imgui.git imgui_src")
    endif()
    if(NOT EXISTS "${IMPLOT_DIR}/implot.h")
        message(FATAL_ERROR
            "ImPlot sources not found at ${IMPLOT_DIR}.\n"
            "Run: cd tools && git clone --depth=1 https://github.com/epezent/implot.git implot_src")
    endif()
    if(NOT EXISTS "${CMAKE_SOURCE_DIR}/tools/miniaudio.h")
        message(FATAL_ERROR
            "miniaudio.h not found at tools/miniaudio.h.\n"
            "Run: curl -L -o tools/miniaudio.h https://raw.githubusercontent.com/mackron/miniaudio/master/miniaudio.h")
    endif()

    set(VIZ_SOURCES
        src/audio_viz_main.cpp
        src/audio_viz_app.cpp
        src/miniaudio_impl.cpp
        # ImGui core
        ${IMGUI_DIR}/imgui.cpp
        ${IMGUI_DIR}/imgui_draw.cpp
        ${IMGUI_DIR}/imgui_tables.cpp
        ${IMGUI_DIR}/imgui_widgets.cpp
        # ImGui GLFW + OpenGL3 backends
        ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
        ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
        # ImPlot
        ${IMPLOT_DIR}/implot.cpp
        ${IMPLOT_DIR}/implot_items.cpp
    )

    add_executable(audio_viz ${VIZ_SOURCES})

    target_include_directories(audio_viz PRIVATE
        include
        ${IMGUI_DIR}
        ${IMGUI_DIR}/backends
        ${IMPLOT_DIR}
        tools
    )

    target_link_libraries(audio_viz
        OpenGL::GL
        glfw
    )

    if(NOT MSVC)
        target_link_libraries(audio_viz m)
    endif()

    # Linux: link dl and pthread for miniaudio
    if(UNIX AND NOT APPLE)
        target_link_libraries(audio_viz dl pthread)
    endif()

    set_target_properties(audio_viz PROPERTIES CXX_STANDARD 17)

    install(TARGETS audio_viz RUNTIME DESTINATION bin)

    message(STATUS "audio_viz visualizer: ENABLED")
endif()
