#!/bin/bash

# Script for building and running the mlrwsmm project on UNIX-like systems
# Make sure this script has execute permissions: chmod +x run.sh

# Exit on any error
set -e

echo "Building Multi-level Reader-Writer Synchronization and Memory Management..."

# Create build directory if it doesn't exist
if [ ! -d "build" ]; then
  mkdir -p build
fi

# Navigate to build directory
cd build

# Run CMake
echo "Running CMake..."
cmake ..

# Build the project
echo "Building project..."
cmake --build .

# Navigate back to project root
cd ..

echo "Build complete! Running main controller..."

# Run the main controller
./build/bin/main