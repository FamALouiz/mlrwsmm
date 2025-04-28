#!/bin/bash

echo "Multi-level Reader-Writer Synchronization and Memory Management System"
echo "Building and running project..."
echo ""

# Create build directory if it doesn't exist
if [ ! -d "./build" ]; then
  mkdir -p "./build"
fi

# Navigate to the build directory
cd "./build"

# Build the project
echo "Building project..."
cmake ..
cmake --build .

# Run the main executable
echo ""
echo "Running main application..."
echo ""
cd ..
./build/bin/main

echo ""
echo "Script execution completed."
read -p "Press Enter to continue..."