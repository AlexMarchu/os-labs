echo "Pulling latest version from git\n"
# git pull

echo -e "Configuring project\n"
cmake -B build

echo -e "Building project\n"
cmake --build build

echo -e "Running project\n"
./build/os

