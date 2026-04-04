# generate_vs_solution.ps1
# Generates a Visual Studio 2022 solution using CMake.
# Usage: .\Scripts\generate_vs_solution.ps1 [-VSVersion 2019|2022]

param(
    [ValidateSet("2019", "2022")]
    [string]$VSVersion = "2022"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

# Navigate to repo root regardless of where the script is invoked from
$RepoRoot = Split-Path -Parent $PSScriptRoot
Push-Location $RepoRoot

try {
    Write-Host "=== NovaForge Visual Studio Solution Generator ===" -ForegroundColor Cyan
    Write-Host ""

    # Check cmake exists
    if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
        Write-Error "cmake not found in PATH. Install CMake 3.20+ and add it to PATH."
        exit 1
    }

    $Generator = switch ($VSVersion) {
        "2019" { "Visual Studio 16 2019" }
        "2022" { "Visual Studio 17 2022" }
    }

    Write-Host "Generator : $Generator"
    Write-Host "Output    : build_vs\NovaForge.sln"
    Write-Host ""

    $cmakeArgs = @(
        "-B", "build_vs",
        "-G", $Generator,
        "-A", "x64",
        "-DCMAKE_CXX_STANDARD=20",
        "-DNF_BUILD_EDITOR=ON",
        "-DNF_BUILD_GAME=ON",
        "-DNF_BUILD_TESTS=ON"
    )

    & cmake @cmakeArgs
    if ($LASTEXITCODE -ne 0) {
        Write-Error "CMake configuration failed with exit code $LASTEXITCODE"
        exit $LASTEXITCODE
    }

    Write-Host ""
    Write-Host "Solution generated successfully." -ForegroundColor Green
    Write-Host "Open: build_vs\NovaForge.sln"
    Write-Host "Or build from CLI: cmake --build build_vs --config Debug"
}
finally {
    Pop-Location
}
