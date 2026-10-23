@echo off
rem ============================================================
rem  run.bat - Double-click to build and play 2048
rem  1. Locate gcc (PATH first, then WinLibs install dir)
rem  2. Compile game.exe from all sources
rem  3. Run the game in this window
rem ============================================================

rem Switch to the directory where this bat file lives
cd /d "%~dp0"

rem --- Step 1: locate gcc ---
where gcc >nul 2>nul
if not errorlevel 1 goto build

set "MINGW_BIN=C:\Users\lenovo\AppData\Local\Microsoft\WinGet\Packages\BrechtSanders.WinLibs.POSIX.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe\mingw64\bin"
if exist "%MINGW_BIN%\gcc.exe" (
    set "PATH=%MINGW_BIN%;%PATH%"
) else (
    echo [ERROR] gcc not found in PATH or the WinLibs directory.
    echo Please install MinGW-w64 first, then run this script again.
    pause
    exit /b 1
)

rem --- Step 2: build ---
:build
echo Building game.exe ...
gcc -Wall -Wextra -std=c11 -O2 -o game.exe main.c game_logic.c ui.c ai.c
if errorlevel 1 (
    echo.
    echo [ERROR] Build failed. Fix the errors above and try again.
    pause
    exit /b 1
)
echo Build succeeded. Starting game...
echo.

rem --- Step 3: run ---
rem The game enables ANSI colors by itself, so colors work in
rem both the classic console and Windows Terminal.
rem After the game exits, pause so the final score stays visible.
cls
game.exe
echo.
echo Game exited. Press any key to close this window...
pause >nul
