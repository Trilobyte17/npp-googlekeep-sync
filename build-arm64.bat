@echo off
REM Build script for ARM64 Windows
REM Requires Visual Studio 2022 with ARM64 components and CMake 3.20+

setlocal EnableDelayedExpansion

echo ==========================================
echo Google Keep Sync Plugin - ARM64 Build
echo ==========================================

REM Check for Visual Studio installation
where cl >nul 2>nul
if %errorlevel% neq 0 (
    echo Error: Visual Studio compiler not found.
    echo Please run this from a Visual Studio ARM64 Developer Command Prompt.
    exit /b 1
)

REM Create build directory
if not exist build mkdir build
cd build

REM Clean previous build
echo Cleaning previous build...
if exist CMakeCache.txt del CMakeCache.txt
if exist CMakeFiles rmdir /s /q CMakeFiles
if exist GoogleKeepSync.* del GoogleKeepSync.*

REM Configure with CMake for ARM64
echo Configuring CMake for ARM64...
cmake -A ARM64 .. -DCMAKE_BUILD_TYPE=Release

if %errorlevel% neq 0 (
    echo Error: CMake configuration failed!
    exit /b 1
)

REM Build
echo Building ARM64 Release...
cmake --build . --config Release

if %errorlevel% neq 0 (
    echo Error: Build failed!
    exit /b 1
)

REM Verify output
if exist bin\Release\GoogleKeepSync.dll (
    echo.
    echo ==========================================
    echo Build successful!
    echo Output: bin\Release\GoogleKeepSync.dll
    echo ==========================================
    
    REM Check architecture
    echo Verify ARM64 architecture:
    dumpbin /headers bin\Release\GoogleKeepSync.dll | findstr "AA64"
) else (
    echo Error: Build output not found!
    exit /b 1
)

cd ..

REM Copy to Notepad++ if installed
set NPP_PATH=%LOCALAPPDATA%\Programs\Notepad++
if exist "%NPP_PATH%\notepad++.exe" (
    echo.
    echo Installing to Notepad++...
    if not exist "%NPP_PATH%\plugins\GoogleKeepSync" mkdir "%NPP_PATH%\plugins\GoogleKeepSync"
    copy /y build\bin\Release\GoogleKeepSync.dll "%NPP_PATH%\plugins\GoogleKeepSync\"
    echo Installed to %NPP_PATH%\plugins\GoogleKeepSync\
)

echo.
echo Done!
pause
