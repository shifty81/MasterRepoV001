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
:: Output is displayed on screen AND written to Saved\Logs\build.log.
::
:: The generated .binlog can be opened with MSBuild Structured Log Viewer:
::   https://msbuildlog.com/
:: ---------------------------------------------------------------------------

:: Ensure we are in the repo root (where NovaForge.sln lives).
cd /d "%~dp0"

:: Create log directory if needed -------------------------------------------
if not exist "Saved\Logs" mkdir "Saved\Logs"
set "LOGFILE=Saved\Logs\build.log"

:: Start log ----------------------------------------------------------------
echo ========================================================== > "%LOGFILE%"
echo  NovaForge build started: %DATE% %TIME%                   >> "%LOGFILE%"
echo ========================================================== >> "%LOGFILE%"

:: Detect VS MSBuild --------------------------------------------------------
where msbuild.exe >nul 2>&1
if errorlevel 1 (
    echo [ERROR] msbuild.exe not found.
    echo         Open a "Developer Command Prompt for VS 2022" and try again.
    echo         Do NOT use "dotnet build" — the .NET SDK MSBuild lacks C++ targets.
    echo.
    echo [ERROR] msbuild.exe not found. >> "%LOGFILE%"
    echo         Open a Developer Command Prompt for VS 2022. >> "%LOGFILE%"
    echo.
    echo Build log saved to: %LOGFILE%
    pause
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
echo === NovaForge %CONFIG% %TARGET% === >> "%LOGFILE%"
echo. >> "%LOGFILE%"

:: Run MSBuild — capture all output to the log file, then display it.
msbuild NovaForge.sln /t:%TARGET% /p:Configuration=%CONFIG% /p:Platform=x64 /m %EXTRA_ARGS% >> "%LOGFILE%" 2>&1
set BUILD_EXIT=!ERRORLEVEL!

:: Show the build output on screen.
type "%LOGFILE%"

if !BUILD_EXIT! neq 0 (
    echo.
    echo [FAILED] Build returned error code !BUILD_EXIT!.
    echo.
    echo Tip: re-run with /bl to capture a binary log for debugging:
    echo   build %CONFIG% /bl
    echo.
    echo Then open the .binlog with https://msbuildlog.com/
    echo.
    echo [FAILED] Build returned error code !BUILD_EXIT! >> "%LOGFILE%"
    echo Build log saved to: %LOGFILE%
    pause
    exit /b !BUILD_EXIT!
)

echo.
echo [OK] NovaForge %CONFIG% %TARGET% succeeded.
echo.
echo [OK] NovaForge %CONFIG% %TARGET% succeeded. >> "%LOGFILE%"
echo Build completed: %DATE% %TIME% >> "%LOGFILE%"
echo Build log saved to: %LOGFILE%
pause
