#!/bin/bash

# Helbreath Server Linux Run Script
# This script runs the Linux server binary

set -e  # Exit on error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}  Helbreath Server - Starting${NC}"
echo -e "${BLUE}========================================${NC}"
echo ""

# Get the directory where this script is located (should be Binaries/Server)
SERVER_DIR="$(cd "$(dirname "$0")" && pwd)"

# Check if server binary exists
if [ ! -f "$SERVER_DIR/Server-Linux" ]; then
    echo -e "${RED}Error: Server binary not found at $SERVER_DIR/Server-Linux${NC}"
    echo -e "${YELLOW}Please build the server first using: ../../build_server.sh${NC}"
    exit 1
fi

# Make sure the binary is executable
chmod +x "$SERVER_DIR/Server-Linux"

# Change to the server directory (important for relative paths in config files)
cd "$SERVER_DIR"

echo -e "${GREEN}Starting Helbreath Server...${NC}"
echo -e "${BLUE}Server directory: $SERVER_DIR${NC}"
echo ""

# Run the server
./Server-Linux

# This will only execute if the server exits
echo ""
echo -e "${YELLOW}Server has stopped.${NC}"
