@echo off
echo Configuring project 
cmake -B build -G "MinGW Makefiles"

echo.
echo Building project
cmake --build build

echo.
echo Running test app
build\test_app.exe

echo.
pause