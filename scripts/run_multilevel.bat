@echo off
echo ================================================================
echo  Multi-Level Generalized Reader-Writer System - Build Script
echo ================================================================

REM Create build directory if it doesn't exist
if not exist build mkdir build

REM Navigate to build directory
cd build

REM Run CMake to generate build files
echo Running CMake...
cmake ..

REM Build the project
echo Building multi-level system...
cmake --build . --config Debug

cd ..


REM Check if build was successful
if %ERRORLEVEL% equ 0 (
    echo ================================================================
    echo  Build successful! Starting multi-level system...
    echo ================================================================
    echo.
    echo System Architecture:
    echo   Level 1: 3 Writers --^> Shared Memory 1
    echo   Level 2: 1 Aggregator ^(Reader L1 --^> Writer L2^) --^> Shared Memory 2
    echo   Level 3: 3 Readers ^<-- Shared Memory 2
    echo.
    echo ================================================================
    .\build\bin\main_multilevel.exe
) else (
    echo ================================================================
    echo  Build failed! Please check the error messages above.
    echo ================================================================
    pause
)

