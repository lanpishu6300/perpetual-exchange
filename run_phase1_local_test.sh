#!/bin/bash

# Phase 1 Optimization Local Test Script (Mac/Linux)
# 直接在本地运行测试，无需Docker

set -e

echo "========================================"
echo "Phase 1 Optimization Local Test"
echo "========================================"
echo ""

# Configuration
THREADS=${1:-8}
DURATION=${2:-30}
ORDERS_PER_SEC=${3:-5000}

echo "Configuration:"
echo "  Threads: $THREADS"
echo "  Duration: $DURATION seconds"
echo "  Orders/sec/thread: $ORDERS_PER_SEC"
echo "  Target TPS: $((THREADS * ORDERS_PER_SEC))"
echo ""

# Check if build directory exists
if [ ! -d "build" ]; then
    echo "Creating build directory..."
    mkdir -p build
fi

cd build

# Configure and build
echo "Configuring CMake..."
cmake .. > /dev/null 2>&1 || cmake ..

echo "Building test_optimized_v3..."
make test_optimized_v3 -j$(sysctl -n hw.ncpu 2>/dev/null || nproc 2>/dev/null || echo 4) 2>&1 | grep -E "(Building|Linking|error|warning)" | head -20

if [ ! -f "test_optimized_v3" ]; then
    echo "❌ Build failed!"
    exit 1
fi

echo ""
echo "Running test..."
echo "========================================"
echo ""

# Run the test
./test_optimized_v3 $THREADS $DURATION $ORDERS_PER_SEC

echo ""
echo "========================================"
echo "Test Complete"
echo "========================================"

