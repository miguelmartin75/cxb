#!/bin/bash

# Development setup script for cxb project
# This script sets up the development environment with all necessary tools

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}Setting up cxb development environment...${NC}"
echo

# Function to check if command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Function to install packages on different systems
install_package() {
    local package="$1"
    
    if command_exists apt-get; then
        echo -e "${YELLOW}Installing $package using apt-get...${NC}"
        sudo apt-get update && sudo apt-get install -y "$package"
    elif command_exists brew; then
        echo -e "${YELLOW}Installing $package using brew...${NC}"
        brew install "$package"
    elif command_exists yum; then
        echo -e "${YELLOW}Installing $package using yum...${NC}"
        sudo yum install -y "$package"
    elif command_exists pacman; then
        echo -e "${YELLOW}Installing $package using pacman...${NC}"
        sudo pacman -S --noconfirm "$package"
    else
        echo -e "${RED}Error: Unable to determine package manager. Please install $package manually.${NC}"
        return 1
    fi
}

# Check and install CMake
if ! command_exists cmake; then
    echo -e "${YELLOW}CMake not found. Installing...${NC}"
    install_package cmake
else
    echo -e "${GREEN}✅ CMake is already installed${NC}"
fi

# Check and install clang-format
if ! command_exists clang-format; then
    echo -e "${YELLOW}clang-format not found. Installing...${NC}"
    install_package clang-format
else
    echo -e "${GREEN}✅ clang-format is already installed${NC}"
fi

# Check and install build essentials
if ! command_exists make; then
    echo -e "${YELLOW}Build tools not found. Installing...${NC}"
    if command_exists apt-get; then
        install_package build-essential
    elif command_exists yum; then
        install_package "Development Tools"
    elif command_exists pacman; then
        install_package base-devel
    else
        echo -e "${RED}Please install build tools manually${NC}"
    fi
else
    echo -e "${GREEN}✅ Build tools are already installed${NC}"
fi

# Install Catch2 v3
echo -e "${YELLOW}Setting up Catch2 v3...${NC}"
if [ ! -d "third_party/Catch2" ]; then
    mkdir -p third_party
    cd third_party
    git clone https://github.com/catchorg/Catch2.git
    cd Catch2
    git checkout v3.4.0
    cmake -Bbuild -H. -DBUILD_TESTING=OFF -DCMAKE_BUILD_TYPE=Release
    sudo cmake --build build/ --target install
    cd ../..
    echo -e "${GREEN}✅ Catch2 v3 installed successfully${NC}"
else
    echo -e "${GREEN}✅ Catch2 is already set up${NC}"
fi

# Set up git hooks (optional)
echo
read -p "Would you like to install the pre-commit formatting hook? (y/N): " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    if [ -f "hooks/pre-commit" ]; then
        cp hooks/pre-commit .git/hooks/pre-commit
        chmod +x .git/hooks/pre-commit
        echo -e "${GREEN}✅ Pre-commit hook installed${NC}"
    else
        echo -e "${RED}Error: hooks/pre-commit not found${NC}"
    fi
fi

# Configure and build the project
echo
echo -e "${YELLOW}Building the project...${NC}"
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build

# Run tests
echo
echo -e "${YELLOW}Running tests...${NC}"
cd build
if command_exists ctest; then
    ctest --output-on-failure
else
    ./test_seq
fi
cd ..

echo
echo -e "${GREEN}🎉 Development environment setup complete!${NC}"
echo
echo -e "${BLUE}Next steps:${NC}"
echo "1. Check code formatting: ./format.sh --check"
echo "2. Format code: ./format.sh"
echo "3. Build project: cmake --build build"
echo "4. Run tests: cd build && ctest"
echo
echo -e "${YELLOW}Happy coding!${NC}"