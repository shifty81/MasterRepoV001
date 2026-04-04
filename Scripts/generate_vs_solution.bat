@echo off
:: generate_vs_solution.bat
:: Generates a Visual Studio 2022 solution using CMake presets.
:: Requires CMake 3.20+ and Visual Studio 2022 installed.

cd /d "%~dp0.."

echo === NovaForge Visual Studio Solution Generator ===
echo.

where cmake >nul 2>&1
if errorlevel 1 (
    echo ERROR: cmake not found in PATH. Install CMake 3.20+ and add it to PATH.
    pause
    exit /b 1
)

:: Default to VS 2022. Override with argument 1: generate_vs_solution.bat 2019
set VS_GENERATOR=Visual Studio 17 2022
if "%1"=="2019" set VS_GENERATOR=Visual Studio 16 2019

echo Generator: %VS_GENERATOR%
echo.

:: Configure with CMake (editor + game + tests)
cmake -B build_vs ^
    -G "%VS_GENERATOR%" ^
    -A x64 ^
    -DCMAKE_CXX_STANDARD=20 ^
    -DNF_BUILD_EDITOR=ON ^
    -DNF_BUILD_GAME=ON ^
    -DNF_BUILD_TESTS=ON

if errorlevel 1 (
    echo.
    echo ERROR: CMake configuration failed.
    pause
    exit /b 1
)

echo.
echo Solution generated: build_vs\NovaForge.sln
echo Open it in Visual Studio or run:
echo   cmake --build build_vs --config Debug
echo.
pause
