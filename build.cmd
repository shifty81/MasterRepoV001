@echo off
setlocal enabledelayedexpansion
:: ---------------------------------------------------------------------------
:: build.cmd — Build NovaForge from a Developer Command Prompt for VS 2022.
::
:: Usage:
::   build              Build Debug (default)
::   build Release      Build Release
::   build Debug /bl    Build Debug with MSBuild binary logging (.binlog)
::   build Rebuild      Rebuild Debug
::
:: The generated .binlog can be opened with MSBuild Structured Log Viewer:
::   https://msbuildlog.com/
:: ---------------------------------------------------------------------------

:: Detect VS MSBuild --------------------------------------------------------
where msbuild.exe >nul 2>&1
if errorlevel 1 (
    echo [ERROR] msbuild.exe not found.
    echo         Open a "Developer Command Prompt for VS 2022" and try again.
    echo         Do NOT use "dotnet build" — the .NET SDK MSBuild lacks C++ targets.
    exit /b 1
)

:: Parse arguments -----------------------------------------------------------
set CONFIG=Debug
set TARGET=Build
set EXTRA_ARGS=

:parse_args
if "%~1"=="" goto done_args
if /i "%~1"=="Debug"   ( set CONFIG=Debug&   shift& goto parse_args )
if /i "%~1"=="Release" ( set CONFIG=Release& shift& goto parse_args )
if /i "%~1"=="Rebuild" ( set TARGET=Rebuild& shift& goto parse_args )
if /i "%~1"=="Clean"   ( set TARGET=Clean&   shift& goto parse_args )
set EXTRA_ARGS=!EXTRA_ARGS! %1
shift
goto parse_args
:done_args

:: Build ---------------------------------------------------------------------
echo.
echo === NovaForge %CONFIG% %TARGET% ===
echo.

msbuild NovaForge.sln /t:%TARGET% /p:Configuration=%CONFIG% /p:Platform=x64 /m %EXTRA_ARGS%
if errorlevel 1 (
    echo.
    echo [FAILED] Build returned error code %ERRORLEVEL%.
    echo.
    echo Tip: re-run with /bl to capture a binary log for debugging:
    echo   build %CONFIG% /bl
    echo.
    echo Then open the .binlog with https://msbuildlog.com/
    exit /b 1
)

echo.
echo [OK] NovaForge %CONFIG% %TARGET% succeeded.
