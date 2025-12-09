#!/bin/bash

# Phase 1 Optimization Docker Test Script

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

# Create results directory
mkdir -p test_results

# Build and run Docker container
echo "Building Docker image..."

# Try docker compose (new version) first, fallback to docker-compose
if command -v docker &> /dev/null && docker compose version &> /dev/null; then
    DOCKER_COMPOSE_CMD="docker compose"
elif command -v docker-compose &> /dev/null; then
    DOCKER_COMPOSE_CMD="docker-compose"
else
    echo "Error: Neither 'docker compose' nor 'docker-compose' found"
    exit 1
fi

$DOCKER_COMPOSE_CMD -f docker-compose.phase1_test.yml build

echo ""
echo "Running test..."
$DOCKER_COMPOSE_CMD -f docker-compose.phase1_test.yml run --rm \
    -e TEST_THREADS=$THREADS \
    -e TEST_DURATION=$DURATION \
    -e TEST_ORDERS_PER_SEC=$ORDERS_PER_SEC \
    phase1_test

echo ""
echo "========================================"
echo "Test Complete"
echo "========================================"
echo ""
echo "Results saved to: test_results/"
echo ""


# Phase 1 Optimization Docker Test Script

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

# Create results directory
mkdir -p test_results

# Build and run Docker container
echo "Building Docker image..."

# Try docker compose (new version) first, fallback to docker-compose
if command -v docker &> /dev/null && docker compose version &> /dev/null; then
    DOCKER_COMPOSE_CMD="docker compose"
elif command -v docker-compose &> /dev/null; then
    DOCKER_COMPOSE_CMD="docker-compose"
else
    echo "Error: Neither 'docker compose' nor 'docker-compose' found"
    exit 1
fi

$DOCKER_COMPOSE_CMD -f docker-compose.phase1_test.yml build

echo ""
echo "Running test..."
$DOCKER_COMPOSE_CMD -f docker-compose.phase1_test.yml run --rm \
    -e TEST_THREADS=$THREADS \
    -e TEST_DURATION=$DURATION \
    -e TEST_ORDERS_PER_SEC=$ORDERS_PER_SEC \
    phase1_test

echo ""
echo "========================================"
echo "Test Complete"
echo "========================================"
echo ""
echo "Results saved to: test_results/"
echo ""

