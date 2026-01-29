#!/bin/bash

# Helbreath Server Linux Build Script
# This script builds the server for Linux/macOS platforms

set -e  # Exit on error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}  Helbreath Server - Linux Build${NC}"
echo -e "${BLUE}========================================${NC}"
echo ""

# Prompt for build type
echo -e "${YELLOW}Select build type:${NC}"
echo "  1) Debug"
echo "  2) Release"
echo -n "Enter choice [1-2]: "
read -r choice

case $choice in
    1)
        BUILD_TYPE="Debug"
        ;;
    2)
        BUILD_TYPE="Release"
        ;;
    *)
        echo -e "${RED}Invalid choice. Defaulting to Debug.${NC}"
        BUILD_TYPE="Debug"
        ;;
esac

echo -e "${GREEN}Building in ${BUILD_TYPE} mode...${NC}"
echo ""

# Navigate to Server directory
cd "$(dirname "$0")/Sources/Server"

# Create build directory if it doesn't exist
if [ ! -d "build" ]; then
    echo -e "${BLUE}Creating build directory...${NC}"
    mkdir -p build
fi

cd build

# Run CMake
echo -e "${BLUE}Running CMake configuration...${NC}"
cmake .. -DCMAKE_BUILD_TYPE=$BUILD_TYPE

# Build
echo -e "${BLUE}Compiling server...${NC}"
if [ "$(uname)" = "Darwin" ]; then
    # macOS
    make -j$(sysctl -n hw.ncpu)
else
    # Linux
    make -j$(nproc)
fi

# Check if build succeeded
if [ $? -ne 0 ]; then
    echo -e "${RED}Build failed!${NC}"
    exit 1
fi

# Create Binaries/Server directory if it doesn't exist
BINARY_DIR="$(dirname "$0")/../../../Binaries/Server"
mkdir -p "$BINARY_DIR"

# Copy binary to Binaries/Server with new name
if [ -f "bin/HBServer" ]; then
    echo -e "${GREEN}Build successful!${NC}"
    echo -e "${BLUE}Copying binary to Binaries/Server...${NC}"
    cp bin/HBServer "$BINARY_DIR/Server-Linux"
    chmod +x "$BINARY_DIR/Server-Linux"
    
    echo ""
    echo -e "${GREEN}========================================${NC}"
    echo -e "${GREEN}  Build Complete!${NC}"
    echo -e "${GREEN}========================================${NC}"
    echo -e "Binary location: ${BLUE}Binaries/Server/Server-Linux${NC}"
    echo -e "Build type: ${BLUE}$BUILD_TYPE${NC}"
    
    # Show binary size
    SIZE=$(ls -lh "$BINARY_DIR/Server-Linux" | awk '{print $5}')
    echo -e "Binary size: ${BLUE}$SIZE${NC}"
    echo ""
else
    echo -e "${RED}Error: Binary not found at bin/HBServer${NC}"
    exit 1
fi
