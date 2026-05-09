#!/bin/bash
# filepath: c:\proj\cpp\kalmanx\build.sh

set -e

# Default values
CONFIG=Release
PLATFORM=x64

# Function to show help
show_help() {
    echo "Usage: build.sh [CONFIG] [PLATFORM]"
    echo ""
    echo "CONFIG:   Debug or Release (default: Release)"
    echo "PLATFORM: x64, or ARM64 (default: x64)"
    echo ""
    echo "Examples:"
    echo "  ./build.sh"
    echo "  ./build.sh Debug"
    echo "  ./build.sh Release x64"
    echo "  ./build.sh Debug ARM64"
    echo "  ./build.sh Release ARM64"
    exit 0
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        Debug|Release)
            CONFIG=$1
            shift
            ;;
        x64|ARM64)
            PLATFORM=$1
            shift
            ;;
        -h|--help)
            show_help
            ;;
        *)
            echo "Unknown argument: $1"
            show_help
            ;;
    esac
done

echo "Building configuration: $CONFIG"
echo "Target platform: $PLATFORM"
echo ""

# Navigate to script directory
cd "$(dirname "$0")"

# Create build directory (platform-specific) if it doesn't exist
BUILD_DIR="build-$PLATFORM"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure CMake project
echo "Configuring CMake project..."
cmake .. -DCMAKE_BUILD_TYPE=$CONFIG \
    -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"

# Build the project
echo "Building project..."
cmake --build . --config $CONFIG

# Detect if cross-compiling
CROSS_COMPILE=0
CURRENT_ARCH=$(uname -m)
if [[ "$PLATFORM" == "ARM64" && "$CURRENT_ARCH" != "arm64" && "$CURRENT_ARCH" != "aarch64" ]]; then
    CROSS_COMPILE=1
fi

if [[ $CROSS_COMPILE -eq 1 ]]; then
    echo "Cross-compiling detected, skipping tests..."
else
    # Run tests
    echo "Running tests..."
    ctest -C $CONFIG --output-on-failure
fi

echo "Build and tests completed successfully!"
