@echo off
echo ========================================
echo Audio Cleaner - Windows Setup
echo ========================================
echo.
echo This will install build tools and compile Audio Cleaner
echo Please run this file as Administrator
echo.
pause

echo.
echo Checking for Chocolatey...
where choco >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo Chocolatey not found. Installing...
    powershell -NoProfile -ExecutionPolicy Bypass -Command "Set-ExecutionPolicy Bypass -Scope Process -Force; [System.Net.ServicePointManager]::SecurityProtocol = [System.Net.ServicePointManager]::SecurityProtocol -bor 3072; iex ((New-Object System.Net.WebClient).DownloadString('https://community.chocolatey.org/install.ps1'))"
    
    echo.
    echo Refreshing environment...
    call refreshenv.cmd
) else (
    echo Chocolatey already installed.
)

echo.
echo Installing MinGW and CMake...
choco install -y mingw cmake git

echo.
echo Refreshing environment...
call refreshenv.cmd

echo.
echo Building Audio Cleaner...
cd /d "%~dp0"
powershell -NoProfile -ExecutionPolicy Bypass -File ".\build_windows.ps1" -BuildOnly

echo.
echo ========================================
echo Build Complete!
echo ========================================
echo.
echo Executable location: build-windows\Release\audio_cleaner.exe
echo.
pause
