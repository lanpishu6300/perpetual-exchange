#!/bin/bash

# Performance Benchmark Script
# Usage: ./run_benchmark.sh [num_orders]

set -e

cd "$(dirname "$0")"
BUILD_DIR="build"
NUM_ORDERS=${1:-10000}

echo "=========================================="
echo "Running Performance Benchmark"
echo "=========================================="
echo "Orders per version: $NUM_ORDERS"
echo ""

if [ ! -f "$BUILD_DIR/comprehensive_performance_comparison" ]; then
    echo "Building benchmark executable..."
    cd "$BUILD_DIR"
    cmake --build . --config Release --target comprehensive_performance_comparison
    cd ..
fi

echo "Starting benchmark..."
echo "This may take a few minutes..."
echo ""

# Run benchmark and save results
"$BUILD_DIR/comprehensive_performance_comparison" "$NUM_ORDERS" | tee benchmark_results_$(date +%Y%m%d_%H%M%S).txt

echo ""
echo "Benchmark completed!"
echo "Results saved to benchmark_results_*.txt"



