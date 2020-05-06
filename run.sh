set -e

# Test for cmake:
if ! [ -x "$(command -v cmake)" ]; then
   read -p "cmake not found (required to configure the build)! Install? (y/n) " -n 1 -r
   echo # move to a new line
   if [[ $REPLY =~ ^[Yy]$ ]]; then
      sudo apt-get update
      sudo apt-get install cmake
   fi
fi

# Test for python:
if ! [ -x "$(command -v python)" ]; then
   read -p "python not found (required to execute the demo app)! Install? (y/n) " -n 1 -r
   echo # move to a new line
   if [[ $REPLY =~ ^[Yy]$ ]]; then
      sudo apt-get update
      sudo apt-get install python
   fi
fi

# Test for gcc
if ! [ -x "$(command -v gcc)" ]; then
   read -p "gcc not found (required to compile the sources)! Install? (y/n) " -n 1 -r
   echo # move to a new line
   if [[ $REPLY =~ ^[Yy]$ ]]; then
      sudo apt-get update
      sudo apt-get install build-essential
   fi
fi

pkgs='qtbase5-dev pyqt5-dev'
if ! dpkg -s $pkgs >/dev/null 2>&1; then
   read -p "One or all of {$pkgs} not found (compilation dependencies)! Install? (y/n) " -n 1 -r
   echo # move to a new line
   if [[ $REPLY =~ ^[Yy]$ ]]; then
      sudo apt-get update
      sudo apt-get install $pkgs
   fi
fi

# Compile:
mkdir -p build
cd build
cmake ..
make -j2
cd ..

# Run:
./build/shared_memory_cpp &
./shared_memory.py
