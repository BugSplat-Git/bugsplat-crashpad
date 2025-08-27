#!/bin/bash
set -e

# Check if depot_tools is in PATH
if ! command -v fetch &> /dev/null; then
    echo "ERROR: depot_tools not found in PATH"
    echo "Please install depot_tools and add it to your PATH:"
    echo "https://commondatastorage.googleapis.com/chrome-infra-docs/flat/depot_tools/docs/html/depot_tools_tutorial.html"
    exit 1
fi

# First build Crashpad
echo "Building Crashpad..."
./scripts/build_crashpad_linux.sh

# Create build directory if it doesn't exist
echo "Building MyCMakeCrasher..."
mkdir -p build
cd build

# Configure and build with Debug mode for better symbol generation
echo "Configuring with Debug symbols for better crash reporting..."
cmake .. -DCMAKE_BUILD_TYPE=Debug
make

echo "Build complete. Run the application with: ./build/Debug/MyCMakeCrasher"

# Return to root directory
cd .. 