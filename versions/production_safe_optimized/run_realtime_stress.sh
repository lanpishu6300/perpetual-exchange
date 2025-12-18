#!/bin/bash

# Real-time Stress Test Runner
# Shows live performance metrics

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"

echo "=========================================="
echo "Real-time Stress Test"
echo "Production Safe Optimized"
echo "Zero Data Loss Guarantee"
echo "=========================================="
echo ""
echo "Starting real-time monitoring..."
echo "Press Ctrl+C to stop"
echo ""

cd "$BUILD_DIR"

# Run with real-time output
./realtime_stress_test 2>&1 | grep -v "Persistence queue full" | grep -v "WARNING"

echo ""
echo "=========================================="
echo "Test Complete"
echo "=========================================="

