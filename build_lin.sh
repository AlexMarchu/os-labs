echo "Configuring project (CMake)"
cmake -B build

echo -e "\nBuilding project"
cmake --build build

echo -e "\nRunning test app"
./build/test_app