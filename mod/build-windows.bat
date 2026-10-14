@echo off
REM Build RO-MP playable mod (ASI) — butuh Visual Studio 2022 + GTA:SA 1.0 US.
REM Jalankan dari "x86 Native Tools Command Prompt" (32-bit!).
cd /d "%~dp0"
if not exist "vendor\plugin-sdk\plugin_sa\plugin_sa.h" (
    echo [1/3] Clone submodules...
    git submodule update --init --recursive
)
echo [2/3] Configure (Win32)...
cmake -B build -G "Visual Studio 17 2022" -A Win32
echo [3/3] Build...
cmake --build build --config Release
echo.
echo OK -^> mod\build\out\Release\romp.dll
echo Copy romp.dll ke folder yang sama dengan romp.exe.
echo (romp.exe otomatis inject ke GTA:SA saat tekan PLAY)
pause
