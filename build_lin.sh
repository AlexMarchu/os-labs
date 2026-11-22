#!/usr/bin/env bash
set -e
echo "Configuring project"
cmake -B build

echo -e "\nBuilding project"
cmake --build build

echo -e "\nRunning test app"
./build/logger