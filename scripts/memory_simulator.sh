#!/bin/bash
echo "Running Memory Management Simulator..."
echo ""

# Default memory size: 1MB (1048576 bytes), Page size: 4KB (4096 bytes)
# Optional parameters: memory_size page_size
# Example: ./memory_simulator.sh 2097152 8192 - for 2MB memory with 8KB pages

cd $(dirname "$0")/..
MEM_SIZE=1048576
PAGE_SIZE=4096

if [ ! -z "$1" ]; then
    MEM_SIZE=$1
fi

if [ ! -z "$2" ]; then
    PAGE_SIZE=$2
fi

./build/bin/memory_simulator $MEM_SIZE $PAGE_SIZE
