@echo off
title Spring Boot Middleware Service (Port 8080)
echo Starting Java 17 Spring Boot Middleware Service on port 8080...
cd /d "%~dp0middleware"

set "JAVA_HOME=C:\Users\kanya\.tools\jdk-17"
set "PATH=C:\Users\kanya\.tools\maven\bin;C:\Users\kanya\.tools\jdk-17\bin;%PATH%"

REM Default to PostgreSQL profile if live postgres is running, or local in-memory H2 if not specified
if "%1"=="" (
    echo Launching with local profile (in-memory PostgreSQL mode)...
    echo Note: To run with external PostgreSQL, run: run-middleware.bat postgres
    mvn spring-boot:run -Dspring-boot.run.profiles=local
) else (
    echo Launching with %1 profile...
    mvn spring-boot:run -Dspring-boot.run.profiles=%1
)

pause
