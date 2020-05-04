set -e

mkdir -p build
cd build
cmake ..
make -j2
cd ..

./build/shared_memory_cpp
