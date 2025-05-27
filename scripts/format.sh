#!/bin/bash

# Format all C/C++ files in the project using clang-format

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Check if clang-format is installed
if ! command -v clang-format &> /dev/null; then
    echo -e "${RED}Error: clang-format is not installed${NC}"
    echo "Please install clang-format first:"
    echo "  Ubuntu/Debian: sudo apt-get install clang-format"
    echo "  macOS: brew install clang-format"
    exit 1
fi

echo "clang-format version: "
echo $(clang-format --version)
echo ""

# Find all C/C++ files, excluding patterns from .gitignore
FILES=$(find . -name "*.cpp" -o -name "*.h" -o -name "*.hpp" -o -name "*.c" | grep -v "./build" | grep -v "./.cache" | grep -v "./prototype")

if [ -z "$FILES" ]; then
    echo -e "${YELLOW}No C/C++ files found to format${NC}"
    exit 0
fi

# Check if --check flag is provided
CHECK_ONLY=false
if [[ "$1" == "--check" ]]; then
    CHECK_ONLY=true
fi

echo -e "${YELLOW}Found files to process:${NC}"
echo "$FILES"
echo

if [ "$CHECK_ONLY" = true ]; then
    echo -e "${YELLOW}Checking formatting (no changes will be made)...${NC}"
    NEEDS_FORMATTING=false
    
    while IFS= read -r file; do
        if ! clang-format --dry-run --Werror "$file" 2>/dev/null; then
            echo -e "${RED}❌ $file needs formatting${NC}"
            NEEDS_FORMATTING=true
        else
            echo -e "${GREEN}✅ $file is properly formatted${NC}"
        fi
    done <<< "$FILES"
    
    if [ "$NEEDS_FORMATTING" = true ]; then
        echo
        echo -e "${YELLOW}Some files need formatting. Run ./scripts/format.sh to fix them.${NC}"
        exit 1
    else
        echo
        echo -e "${GREEN}All files are properly formatted!${NC}"
        exit 0
    fi
else
    echo -e "${YELLOW}Formatting files...${NC}"
    
    while IFS= read -r file; do
        echo "Formatting $file"
        clang-format -i "$file"
    done <<< "$FILES"
    
    echo
    echo -e "${GREEN}All files have been formatted!${NC}"
fi
