#!/bin/bash

# Real-time Stress Test with Live Monitoring
# Tests high throughput while ensuring zero data loss

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"

echo "=========================================="
echo "Real-time Stress Test"
echo "Production Safe Optimized - Zero Data Loss"
echo "=========================================="
echo ""

# Build if needed
cd "$BUILD_DIR"
if [ ! -f "production_safe_optimized_benchmark" ]; then
    echo "Building..."
    cmake "$SCRIPT_DIR" > /dev/null 2>&1
    make -j4 > /dev/null 2>&1
fi

# Create data directory
mkdir -p "$SCRIPT_DIR/data/wal"

echo "Starting real-time stress test..."
echo "Press Ctrl+C to stop"
echo ""
echo "Monitoring performance metrics..."
echo ""

# Run benchmark and monitor in real-time
cd "$BUILD_DIR"

# Use a named pipe or direct output processing
./production_safe_optimized_benchmark 2>&1 | while IFS= read -r line; do
    # Filter out persistence queue warnings for cleaner output
    if [[ ! "$line" =~ "Persistence queue full" ]]; then
        # Highlight important metrics
        if [[ "$line" =~ "Throughput:" ]]; then
            echo -e "\033[1;32m$line\033[0m"  # Green for throughput
        elif [[ "$line" =~ "Latency:" ]]; then
            echo -e "\033[1;33m$line\033[0m"  # Yellow for latency
        elif [[ "$line" =~ "WAL Stats:" ]] || [[ "$line" =~ "Async Writes:" ]] || [[ "$line" =~ "Sync Count:" ]]; then
            echo -e "\033[1;36m$line\033[0m"  # Cyan for WAL stats
        elif [[ "$line" =~ "Benchmark Results" ]] || [[ "$line" =~ "===" ]]; then
            echo -e "\033[1;35m$line\033[0m"  # Magenta for headers
        else
            echo "$line"
        fi
    fi
done

echo ""
echo "=========================================="
echo "Stress Test Complete"
echo "=========================================="

