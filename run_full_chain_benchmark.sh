#!/bin/bash

# Full Chain Performance Benchmark Script
# Runs comprehensive performance tests and generates reports

set -e

echo "========================================"
echo "Full Chain Performance Benchmark"
echo "========================================"
echo ""

# Configuration
THREADS=${1:-16}
DURATION=${2:-60}
ORDERS_PER_THREAD=${3:-62500}  # 16 * 62500 = 1M TPS target
ENABLE_PERSISTENCE=${4:-1}
ENABLE_EVENT_SOURCING=${5:-1}

echo "Configuration:"
echo "  Threads: $THREADS"
echo "  Duration: $DURATION seconds"
echo "  Orders per thread/sec: $ORDERS_PER_THREAD"
echo "  Target TPS: $((THREADS * ORDERS_PER_THREAD))"
echo "  Persistence: $([ "$ENABLE_PERSISTENCE" = "1" ] && echo "ON" || echo "OFF")"
echo "  Event Sourcing: $([ "$ENABLE_EVENT_SOURCING" = "1" ] && echo "ON" || echo "OFF")"
echo ""

# Build
echo "Building benchmark..."
cd build
cmake .. > /dev/null 2>&1
make full_chain_benchmark -j$(nproc) > /dev/null 2>&1

# Run benchmark
echo "Running benchmark..."
./full_chain_benchmark $THREADS $DURATION $ORDERS_PER_THREAD $ENABLE_PERSISTENCE $ENABLE_EVENT_SOURCING

# Run bottleneck analyzer
echo ""
echo "Running bottleneck analyzer..."
make bottleneck_analyzer -j$(nproc) > /dev/null 2>&1
./bottleneck_analyzer

echo ""
echo "========================================"
echo "Benchmark Complete"
echo "========================================"
echo ""
echo "Reports generated:"
echo "  - full_chain_benchmark_report.md"
echo "  - optimization_recommendations.md"
echo "  - bottleneck_analysis_report.md"
echo ""
echo "Review these reports for performance analysis and optimization recommendations."


# Full Chain Performance Benchmark Script
# Runs comprehensive performance tests and generates reports

set -e

echo "========================================"
echo "Full Chain Performance Benchmark"
echo "========================================"
echo ""

# Configuration
THREADS=${1:-16}
DURATION=${2:-60}
ORDERS_PER_THREAD=${3:-62500}  # 16 * 62500 = 1M TPS target
ENABLE_PERSISTENCE=${4:-1}
ENABLE_EVENT_SOURCING=${5:-1}

echo "Configuration:"
echo "  Threads: $THREADS"
echo "  Duration: $DURATION seconds"
echo "  Orders per thread/sec: $ORDERS_PER_THREAD"
echo "  Target TPS: $((THREADS * ORDERS_PER_THREAD))"
echo "  Persistence: $([ "$ENABLE_PERSISTENCE" = "1" ] && echo "ON" || echo "OFF")"
echo "  Event Sourcing: $([ "$ENABLE_EVENT_SOURCING" = "1" ] && echo "ON" || echo "OFF")"
echo ""

# Build
echo "Building benchmark..."
cd build
cmake .. > /dev/null 2>&1
make full_chain_benchmark -j$(nproc) > /dev/null 2>&1

# Run benchmark
echo "Running benchmark..."
./full_chain_benchmark $THREADS $DURATION $ORDERS_PER_THREAD $ENABLE_PERSISTENCE $ENABLE_EVENT_SOURCING

# Run bottleneck analyzer
echo ""
echo "Running bottleneck analyzer..."
make bottleneck_analyzer -j$(nproc) > /dev/null 2>&1
./bottleneck_analyzer

echo ""
echo "========================================"
echo "Benchmark Complete"
echo "========================================"
echo ""
echo "Reports generated:"
echo "  - full_chain_benchmark_report.md"
echo "  - optimization_recommendations.md"
echo "  - bottleneck_analysis_report.md"
echo ""
echo "Review these reports for performance analysis and optimization recommendations."

