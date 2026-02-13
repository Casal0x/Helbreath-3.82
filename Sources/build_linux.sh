#!/bin/bash
# Linux build script for Helbreath Server
# Usage:
#   ./build_linux.sh          Build (incremental)
#   ./build_linux.sh clean    Clean and rebuild from scratch
#   ./build_linux.sh release  Build in Release mode

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/Server/build"
BUILD_TYPE="${1:-Debug}"

if [ "$1" = "clean" ]; then
    echo "Cleaning build directory..."
    rm -rf "$BUILD_DIR"
    echo "Done."
    exit 0
fi

if [ "$1" = "release" ]; then
    BUILD_TYPE="Release"
fi

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

if [ ! -f CMakeCache.txt ]; then
    echo "Configuring (${BUILD_TYPE})..."
    cmake "$SCRIPT_DIR/Server" -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
fi

echo "Building..."
make -j2

echo ""
echo "Build complete: $BUILD_DIR/HelbreathServer"
