#!/bin/bash

# Helbreath Build Script
# Run from Git Bash or any bash shell on Windows

BUILD_SCRIPT="Sources/build.ps1"
CONFIG="Debug"

echo "=================================="
echo "  Helbreath Build Menu"
echo "=================================="
echo "1) All      - Build everything (Server + DDraw + SFML clients)"
echo "2) Server   - Build Server only"
echo "3) DDraw    - Build DDraw client only"
echo "4) SFML     - Build SFML client only"
echo "=================================="
echo -n "Select option [1-4]: "
read option

case $option in
    1)
        echo ""
        echo "Building ALL (Server + DDraw + SFML)..."
        powershell.exe -ExecutionPolicy Bypass -File "$BUILD_SCRIPT" -Target All -Config $CONFIG
        ;;
    2)
        echo ""
        echo "Building Server..."
        powershell.exe -ExecutionPolicy Bypass -File "$BUILD_SCRIPT" -Target Server -Config $CONFIG
        ;;
    3)
        echo ""
        echo "Building DDraw client..."
        powershell.exe -ExecutionPolicy Bypass -File "$BUILD_SCRIPT" -Target Game -Config $CONFIG -Renderer DDraw
        ;;
    4)
        echo ""
        echo "Building SFML client..."
        powershell.exe -ExecutionPolicy Bypass -File "$BUILD_SCRIPT" -Target Game -Config $CONFIG -Renderer SFML
        ;;
    *)
        echo ""
        echo "Invalid option. Please select 1-4."
        exit 1
        ;;
esac

echo ""
echo "Build completed! Check the logs for details:"
echo "  - build_game.log"
echo "  - build_server.log"
echo "  - build_all.log"