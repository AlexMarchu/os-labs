@echo off
echo === Pulling latest changes from git ===
git pull

echo.
echo === Configuring project (CMake) ===
cmake -B build -G "MinGW Makefiles"

echo.
echo === Building project ===
cmake --build build

echo.
echo === Running project ===
.\build\os.exe

echo.
pause