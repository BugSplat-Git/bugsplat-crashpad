#!/bin/bash
# Script to upload symbols to BugSplat

# Get the script directory
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
ROOT_DIR="$(dirname "$SCRIPT_DIR")"

# Check if .env file exists
if [ -f "${ROOT_DIR}/.env" ]; then
    echo "Loading environment from .env file..."
    source "${ROOT_DIR}/.env"
else
    echo "Warning: .env file not found. Make sure you have set BUGSPLAT_CLIENT_ID and BUGSPLAT_CLIENT_SECRET environment variables."
fi

# Check if credentials are set
if [ -z "$BUGSPLAT_CLIENT_ID" ] || [ -z "$BUGSPLAT_CLIENT_SECRET" ]; then
    echo "Error: BugSplat credentials not found."
    echo "Please set BUGSPLAT_CLIENT_ID and BUGSPLAT_CLIENT_SECRET environment variables or create a .env file."
    exit 1
fi

# Determine build directory and ensure we're looking in the Debug subdirectory
BUILD_DIR="${ROOT_DIR}/build"
DEBUG_DIR="${BUILD_DIR}/Debug"
if [ ! -d "$DEBUG_DIR" ]; then
    echo "Error: Debug directory not found. Please build the project in Debug configuration first."
    exit 1
fi

# Extract configuration from src/main.h
MAIN_H="${ROOT_DIR}/src/main.h"
if [ ! -f "$MAIN_H" ]; then
    echo "Error: src/main.h not found."
    exit 1
fi

# Extract values from src/main.h - macOS compatible using sed
DATABASE=$(grep "^#define BUGSPLAT_DATABASE" "$MAIN_H" | sed -E 's/.*"([^"]+)".*/\1/')
APP_NAME=$(grep "^#define BUGSPLAT_APP_NAME" "$MAIN_H" | sed -E 's/.*"([^"]+)".*/\1/')
APP_VERSION=$(grep "^#define BUGSPLAT_APP_VERSION" "$MAIN_H" | sed -E 's/.*"([^"]+)".*/\1/')

if [ -z "$DATABASE" ] || [ -z "$APP_NAME" ] || [ -z "$APP_VERSION" ]; then
    echo "Error: Could not extract all required values from src/main.h."
    echo "DATABASE='$DATABASE'"
    echo "APP_NAME='$APP_NAME'"
    echo "APP_VERSION='$APP_VERSION'"
    exit 1
fi

echo "Database: $DATABASE"
echo "App Name: $APP_NAME"
echo "Version: $APP_VERSION"

# Determine platform and run appropriate script
if [ "$(uname)" == "Darwin" ]; then
    echo "Detected macOS platform"
    UPLOAD_SCRIPT="${SCRIPT_DIR}/symbol_upload_macos.sh"
else
    echo "Detected Linux platform"
    UPLOAD_SCRIPT="${SCRIPT_DIR}/symbol_upload_linux.sh"
fi

# Execute the platform-specific script
echo "Running symbol upload script: $UPLOAD_SCRIPT"
"$UPLOAD_SCRIPT" "$DATABASE" "$APP_NAME" "$APP_VERSION" "$DEBUG_DIR" "$BUGSPLAT_CLIENT_ID" "$BUGSPLAT_CLIENT_SECRET"

# Check result
if [ $? -eq 0 ]; then
    echo "Symbol upload completed successfully."
else
    echo "Symbol upload failed. Please check your credentials and network connection."
    exit 1
fi 