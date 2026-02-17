@echo off
echo Building Audio Cleaner with Plugin Architecture...

REM Try to find Visual Studio C++ compiler
set "VS_PATH="
for %%v in (2022 2019 2017) do (
    if exist "C:\Program Files\Microsoft Visual Studio\%%v\Community\VC\Auxiliary\Build\vcvars64.bat" (
        set "VS_PATH=C:\Program Files\Microsoft Visual Studio\%%v\Community\VC\Auxiliary\Build\vcvars64.bat"
        goto :found
    )
    if exist "C:\Program Files (x86)\Microsoft Visual Studio\%%v\Community\VC\Auxiliary\Build\vcvars64.bat" (
        set "VS_PATH=C:\Program Files (x86)\Microsoft Visual Studio\%%v\Community\VC\Auxiliary\Build\vcvars64.bat"
        goto :found
    )
)

:found
if "%VS_PATH%"=="" (
    echo Visual Studio C++ compiler not found. Please install Visual Studio with C++ development tools.
    pause
    exit /b 1
)

echo Found Visual Studio at: %VS_PATH%
call "%VS_PATH%"

REM Compile the program with all format plugins
cl /EHsc /W4 /O2 /Iinclude ^
   src\main.cpp ^
   src\wav_format.cpp ^
   src\mp3_format.cpp ^
   src\ogg_format.cpp ^
   src\audio_loader.cpp ^
   src\audio_processor.cpp ^
   /Fe:audio_cleaner.exe

if %ERRORLEVEL% EQU 0 (
    echo Build successful!
    echo Executable: audio_cleaner.exe
    echo.
    echo Plugin architecture loaded with support for:
    echo   - WAV (full support)
    echo   - MP3 (framework - requires external library)
    echo   - OGG (framework - requires external library)
) else (
    echo Build failed!
    pause
    exit /b 1
)

pause
