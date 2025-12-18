#!/bin/bash

# Script to run benchmarks in Docker with real-time output
# Usage: ./run_benchmark_realtime.sh [num_orders]

set -e

NUM_ORDERS=${1:-50000}
REPORTS_DIR="./benchmark_reports"

echo "=========================================="
echo "Docker Benchmark Runner - Real-time Output"
echo "=========================================="
echo "Number of orders: $NUM_ORDERS"
echo "Reports will be saved to: $REPORTS_DIR"
echo ""

# Check Docker
if ! docker info > /dev/null 2>&1; then
    echo "❌ Error: Docker daemon is not running"
    exit 1
fi

# Detect docker compose command
if docker compose version > /dev/null 2>&1; then
    DOCKER_COMPOSE="docker compose"
elif docker-compose version > /dev/null 2>&1; then
    DOCKER_COMPOSE="docker-compose"
else
    echo "❌ Error: docker compose not available"
    exit 1
fi

mkdir -p "$REPORTS_DIR"

# Build image in background if needed, or use existing
echo "🔨 Checking Docker image..."
if docker images | grep -q "perpetual-exchange.*benchmark"; then
    echo "✅ Using existing image"
else
    echo "🔨 Building Docker image (this may take a few minutes)..."
    $DOCKER_COMPOSE -f docker-compose.benchmark.yml build benchmark-runner 2>&1 | grep -E "Building|Built|ERROR|error" || true
fi

echo ""
echo "🚀 Running benchmarks with real-time output..."
echo "=========================================="
echo ""

# Run benchmarks with real-time output
$DOCKER_COMPOSE -f docker-compose.benchmark.yml run --rm \
    -e NUM_ORDERS="$NUM_ORDERS" \
    benchmark-runner bash -c "
    echo 'Starting benchmarks for all versions...'
    echo ''
    /app/run_all_benchmarks.sh $NUM_ORDERS
    echo ''
    echo 'Generating comprehensive report...'
    /app/generate_benchmark_report.sh || true
" 2>&1 | tee docker_benchmark_realtime.log

echo ""
echo "=========================================="
echo "Benchmark Results"
echo "=========================================="
echo ""
echo "Reports saved to: $REPORTS_DIR"
ls -lh "$REPORTS_DIR"/*.md 2>/dev/null | head -12 || echo "No reports found"

echo ""
echo "✅ Benchmarks completed!"
echo ""
echo "View reports:"
echo "  cat $REPORTS_DIR/BENCHMARK_COMPARISON_ANALYSIS.md"

