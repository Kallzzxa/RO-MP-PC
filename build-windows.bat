@echo off
REM Build RO-MP PC di Windows. Butuh: Visual Studio 2022 (MSVC), CMake, Git.
REM Jalankan dari "x64 Native Tools Command Prompt".
cd /d "%~dp0"
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j %NUMBER_OF_PROCESSORS%
echo.
echo OK -^> .\build\Release\romp-pc.exe
pause
