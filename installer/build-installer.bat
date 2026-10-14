@echo off
REM Build romp-setup.exe — butuh NSIS 3.x (https://nsis.sourceforge.io)
REM + hasil build launcher & mod:
REM   ..\build\Release\romp.exe   (dari build-windows.bat di root)
REM   ..\mod\build\out\Release\romp.dll  (dari mod\build-windows.bat)
cd /d "%~dp0"
if not exist "..\build\Release\romp.exe" (
    echo ERROR: ..\build\Release\romp.exe tidak ada. Build launcher dulu.
    pause & exit /b 1
)
if not exist "..\mod\build\out\Release\romp.dll" (
    echo ERROR: ..\mod\build\out\Release\romp.dll tidak ada. Build mod dulu.
    pause & exit /b 1
)
makensis romp-setup.nsi
echo.
echo OK -^> installer\romp-setup.exe
pause
