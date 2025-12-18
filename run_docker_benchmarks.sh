#!/bin/bash

# Script to run all benchmarks in Docker
# Usage: ./run_docker_benchmarks.sh [num_orders]

set -e

NUM_ORDERS=${1:-50000}
REPORTS_DIR="./benchmark_reports"

echo "=========================================="
echo "Docker Benchmark Runner"
echo "=========================================="
echo "Number of orders: $NUM_ORDERS"
echo "Reports will be saved to: $REPORTS_DIR"
echo ""

# Check if Docker is running
if ! docker info > /dev/null 2>&1; then
    echo "❌ Error: Docker daemon is not running"
    echo ""
    echo "Please start Docker:"
    echo "  macOS: Open Docker Desktop"
    echo "  Linux: sudo systemctl start docker"
    exit 1
fi

echo "✅ Docker daemon is running"
echo ""

# Create reports directory
mkdir -p "$REPORTS_DIR"

# Detect docker compose command (v2 uses 'docker compose', v1 uses 'docker-compose')
if docker compose version > /dev/null 2>&1; then
    DOCKER_COMPOSE="docker compose"
elif docker-compose version > /dev/null 2>&1; then
    DOCKER_COMPOSE="docker-compose"
else
    echo "❌ Error: Neither 'docker compose' nor 'docker-compose' is available"
    exit 1
fi

echo "Using: $DOCKER_COMPOSE"
echo ""

# Build Docker image
echo "🔨 Building Docker image..."
$DOCKER_COMPOSE -f docker-compose.benchmark.yml build benchmark-runner || {
    echo "❌ Build failed"
    exit 1
}

echo "✅ Docker image built successfully"
echo ""

# Run benchmarks with real-time output
echo "🚀 Running benchmarks for all versions..."
echo "=========================================="
echo "This will show real-time progress..."
echo ""
$DOCKER_COMPOSE -f docker-compose.benchmark.yml run --rm benchmark-runner /app/run_all_benchmarks.sh "$NUM_ORDERS" 2>&1 | tee benchmark_run_output.log || {
    echo "⚠️  Some benchmarks may have failed, but continuing..."
}

echo ""
echo "=========================================="
echo "Generating comprehensive report..."
echo "=========================================="

# Generate comprehensive report
$DOCKER_COMPOSE -f docker-compose.benchmark.yml run --rm benchmark-runner /app/generate_benchmark_report.sh || {
    echo "⚠️  Report generation may have issues, but reports are available in $REPORTS_DIR"
}

echo ""
echo "=========================================="
echo "Benchmark Results"
echo "=========================================="
echo ""
echo "Reports saved to: $REPORTS_DIR"
echo ""
ls -lh "$REPORTS_DIR"/*.md 2>/dev/null | head -15 || echo "No reports found"

echo ""
echo "✅ All benchmarks completed!"
echo ""
echo "To view reports:"
echo "  cat $REPORTS_DIR/COMPREHENSIVE_BENCHMARK_REPORT.md"
echo "  cat $REPORTS_DIR/*_BENCHMARK_REPORT.md"

