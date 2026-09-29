@echo off
title Launch All 3 Tiers
echo ==========================================================
echo   Launching 3-Tier Report Generator Application
echo ==========================================================
echo 1. Starting C++ Native Rendering Engine on port 8081...
start "C++ Renderer (Port 8081)" cmd /k "%~dp0run-cpp.bat"

timeout /t 2 >nul

echo 2. Starting Spring Boot Middleware on port 8080...
start "Spring Boot Middleware (Port 8080)" cmd /k "%~dp0run-middleware.bat"

timeout /t 5 >nul

echo 3. Starting Angular Frontend on port 4200...
start "Angular Frontend (Port 4200)" cmd /k "%~dp0run-frontend.bat"

echo ==========================================================
echo All 3 services are launching in their own terminal windows:
echo - Frontend:   http://localhost:4200
echo - Middleware: http://localhost:8080 (Swagger: http://localhost:8080/swagger-ui.html)
echo - C++ Engine: http://localhost:8081/health
echo ==========================================================
