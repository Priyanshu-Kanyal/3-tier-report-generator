@echo off
title C++ Report Rendering Service (Port 8081)
echo Starting C++ Native Report Rendering Service on port 8081...
cd /d "%~dp0backend-cpp"

if not exist render_service.exe (
    echo Compiling render_service.exe...
    set "PATH=C:\Users\kanya\.tools\w64devkit\bin;%PATH%"
    g++ -std=c++17 -O2 src/main.cpp -o render_service.exe -lws2_32
)

render_service.exe 8081
pause
