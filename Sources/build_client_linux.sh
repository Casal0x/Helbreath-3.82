#!/bin/bash
# Linux build script for Helbreath Client
# Usage:
#   ./build_client_linux.sh                 Build (incremental, Debug)
#   ./build_client_linux.sh debug           Build Debug
#   ./build_client_linux.sh release         Build Release
#   ./build_client_linux.sh clean           Clean build directory and exit
#   ./build_client_linux.sh clean debug     Clean then rebuild Debug
#   ./build_client_linux.sh clean release   Clean then rebuild Release
#
# Output mirrors the Windows layout:
#   Sources/Debug/HelbreathClient    (debug build)
#   Sources/Release/HelbreathClient  (release build)
#
# Prerequisites (Ubuntu/Debian):
#   sudo apt install cmake g++ libx11-dev libxrandr-dev libxcursor-dev \
#       libgl1-mesa-dev libudev-dev libfreetype-dev
#
# SFML 3 is fetched and built automatically via CMake FetchContent.
# If you have SFML 3 installed system-wide, it will be used instead.

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/Client/build"
DO_CLEAN=false
BUILD_TYPE="Debug"

# Parse arguments
for arg in "$@"; do
    case "$arg" in
        clean)   DO_CLEAN=true ;;
        debug)   BUILD_TYPE="Debug" ;;
        release) BUILD_TYPE="Release" ;;
        *)       echo "Unknown argument: $arg"; echo "Usage: $0 [clean] [debug|release]"; exit 1 ;;
    esac
done

# Clean
if [ "$DO_CLEAN" = true ]; then
    echo "Cleaning build directory..."
    rm -rf "$BUILD_DIR"
    echo "Done."
    # If no build type was explicitly requested alongside clean, just exit
    if [ $# -eq 1 ]; then
        exit 0
    fi
fi

# Check if existing build was configured with a different build type
if [ -f "$BUILD_DIR/CMakeCache.txt" ]; then
    CACHED_TYPE=$(grep -oP 'CMAKE_BUILD_TYPE:STRING=\K.*' "$BUILD_DIR/CMakeCache.txt" 2>/dev/null || true)
    if [ -n "$CACHED_TYPE" ] && [ "$CACHED_TYPE" != "$BUILD_TYPE" ]; then
        echo "Build type changed ($CACHED_TYPE -> $BUILD_TYPE), reconfiguring..."
        rm -rf "$BUILD_DIR"
    fi
fi

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

if [ ! -f CMakeCache.txt ]; then
    echo "Configuring (${BUILD_TYPE})..."
    cmake "$SCRIPT_DIR/Client" -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
fi

echo "Building (${BUILD_TYPE})..."
make -j$(nproc)

# Copy binary to Sources/Debug or Sources/Release to mirror Windows layout
OUTPUT_DIR="$SCRIPT_DIR/$BUILD_TYPE"
mkdir -p "$OUTPUT_DIR"
cp "$BUILD_DIR/HelbreathClient" "$OUTPUT_DIR/HelbreathClient"

echo ""
echo "Build complete: $OUTPUT_DIR/HelbreathClient"
