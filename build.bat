@echo off
rem ShroomVault build script - uses the bundled portable toolchain in tools\
rem   build.bat        = game build (no console window)
rem   build.bat dev    = dev build (keeps console for logs)
setlocal
set "DK=%~dp0tools\w64devkit\bin"
set "FLAGS=-O2 -std=c++17 -Wall -Wextra -Wno-missing-field-initializers -Wno-unused-function -Wno-unused-parameter"
set "LINK=-Lraylib\lib -lraylib -lopengl32 -lgdi32 -lwinmm -static -static-libgcc -static-libstdc++"
set "SUBSYS=-mwindows"
if /I "%~1"=="dev" set "SUBSYS="

set "PATH=%DK%;%PATH%"
"%DK%\g++.exe" "%~dp0src\main.cpp" -o "%~dp0ShroomVault.exe" -I"%~dp0raylib\include" %FLAGS% %SUBSYS% %LINK%
if errorlevel 1 (
    echo BUILD FAILED
    exit /b 1
)
echo Built ShroomVault.exe
