#!/bin/bash
set -e

# Create third_party directory if it doesn't exist
mkdir -p third_party
cd third_party

# Check if crashpad directory already exists
if [ ! -d "crashpad" ]; then
  echo "Fetching Crashpad using depot_tools..."
  fetch crashpad
  cd crashpad
else
  echo "Crashpad already exists, updating..."
  cd crashpad
  git checkout main
  git pull
  gclient sync
fi

# CRASHPAD_COMMIT pins the build to one commit (the monthly workflow builds every platform from
# the same one). Unset, the build uses the head of main.
if [ -n "${CRASHPAD_COMMIT:-}" ]; then
  echo "Checking out Crashpad $CRASHPAD_COMMIT..."
  git checkout -q "$CRASHPAD_COMMIT"
  gclient sync
fi

# Generate build files with GN for Debug configuration
echo "Generating Debug build files with GN..."
gn gen out/macos-debug --args="is_debug=true"

# Generate build files with GN for Release configuration
echo "Generating Release build files with GN..."
gn gen out/macos-release --args="is_debug=false"

# Build Debug
echo "Building Debug with Ninja..."
ninja -C out/macos-debug

# Build Release
echo "Building Release with Ninja..."
ninja -C out/macos-release

echo "Crashpad Debug and Release builds complete." 