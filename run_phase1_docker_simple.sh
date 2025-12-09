#!/bin/bash

# Phase 1 Optimization Docker Test Script (Simple version using docker directly)

set -e

echo "========================================"
echo "Phase 1 Optimization Docker Test"
echo "========================================"
echo ""

# Configuration
THREADS=${1:-16}
DURATION=${2:-60}
ORDERS_PER_SEC=${3:-10000}

echo "Configuration:"
echo "  Threads: $THREADS"
echo "  Duration: $DURATION seconds"
echo "  Orders/sec/thread: $ORDERS_PER_SEC"
echo "  Target TPS: $((THREADS * ORDERS_PER_SEC))"
echo ""

# Check if Docker is available
if ! command -v docker &> /dev/null; then
    echo "Error: Docker is not installed or not in PATH"
    exit 1
fi

# Create results directory
mkdir -p test_results

# Build Docker image
echo "Building Docker image..."
docker build -f Dockerfile.phase1_test -t phase1_test:latest .

echo ""
echo "Running test..."
docker run --rm \
    -e TEST_THREADS=$THREADS \
    -e TEST_DURATION=$DURATION \
    -e TEST_ORDERS_PER_SEC=$ORDERS_PER_SEC \
    -v "$(pwd)/test_results:/app/test_results" \
    --shm-size=2g \
    phase1_test:latest \
    sh -c "
        echo '=== Phase 1 Optimization Test ===' &&
        echo 'Threads: ${TEST_THREADS}' &&
        echo 'Duration: ${TEST_DURATION} seconds' &&
        echo 'Orders/sec/thread: ${TEST_ORDERS_PER_SEC}' &&
        echo '' &&
        cd /app/build &&
        ./test_optimized_v3 ${TEST_THREADS} ${TEST_DURATION} ${TEST_ORDERS_PER_SEC} &&
        echo '' &&
        echo '=== Test Complete ==='
    "

echo ""
echo "========================================"
echo "Test Complete"
echo "========================================"
echo ""
echo "Results saved to: test_results/"
echo ""


# Phase 1 Optimization Docker Test Script (Simple version using docker directly)

set -e

echo "========================================"
echo "Phase 1 Optimization Docker Test"
echo "========================================"
echo ""

# Configuration
THREADS=${1:-16}
DURATION=${2:-60}
ORDERS_PER_SEC=${3:-10000}

echo "Configuration:"
echo "  Threads: $THREADS"
echo "  Duration: $DURATION seconds"
echo "  Orders/sec/thread: $ORDERS_PER_SEC"
echo "  Target TPS: $((THREADS * ORDERS_PER_SEC))"
echo ""

# Check if Docker is available
if ! command -v docker &> /dev/null; then
    echo "Error: Docker is not installed or not in PATH"
    exit 1
fi

# Create results directory
mkdir -p test_results

# Build Docker image
echo "Building Docker image..."
docker build -f Dockerfile.phase1_test -t phase1_test:latest .

echo ""
echo "Running test..."
docker run --rm \
    -e TEST_THREADS=$THREADS \
    -e TEST_DURATION=$DURATION \
    -e TEST_ORDERS_PER_SEC=$ORDERS_PER_SEC \
    -v "$(pwd)/test_results:/app/test_results" \
    --shm-size=2g \
    phase1_test:latest \
    sh -c "
        echo '=== Phase 1 Optimization Test ===' &&
        echo 'Threads: ${TEST_THREADS}' &&
        echo 'Duration: ${TEST_DURATION} seconds' &&
        echo 'Orders/sec/thread: ${TEST_ORDERS_PER_SEC}' &&
        echo '' &&
        cd /app/build &&
        ./test_optimized_v3 ${TEST_THREADS} ${TEST_DURATION} ${TEST_ORDERS_PER_SEC} &&
        echo '' &&
        echo '=== Test Complete ==='
    "

echo ""
echo "========================================"
echo "Test Complete"
echo "========================================"
echo ""
echo "Results saved to: test_results/"
echo ""

