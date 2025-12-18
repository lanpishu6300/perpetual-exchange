#!/bin/bash

# Run all benchmarks on Mac
# Usage: ./run_mac_benchmarks.sh [num_orders]

set -e

# Get script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

NUM_ORDERS=${1:-50000}
REPORTS_DIR="./benchmark_reports/mac"
mkdir -p "$REPORTS_DIR"

echo "=========================================="
echo "Running Mac Benchmarks"
echo "Platform: macOS"
echo "CPU Cores: $(sysctl -n hw.ncpu)"
echo "Test Orders: $NUM_ORDERS"
echo "=========================================="
echo ""

VERSIONS=(
    "original"
    "optimized"
    "optimized_v2"
    "art"
    "art_simd"
    "event_sourcing"
    "production_basic"
    "production_fast"
    "production_safe"
    "production_safe_optimized"
)

SUCCESSFUL=()
FAILED=()

for version in "${VERSIONS[@]}"; do
    echo "----------------------------------------"
    echo "Running benchmark for: $version"
    echo "----------------------------------------"
    
    VERSION_DIR="versions/$version"
    BUILD_DIR="$VERSION_DIR/build"
    BENCHMARK_BIN="$BUILD_DIR/${version}_benchmark"
    
    if [ ! -d "$VERSION_DIR" ]; then
        echo "  ⚠️  Directory not found: $VERSION_DIR"
        FAILED+=("$version (not found)")
        continue
    fi
    
    # Always rebuild on Mac to ensure native binaries
    echo "  Building $version benchmark for Mac..."
    cd "$SCRIPT_DIR/$VERSION_DIR"
    rm -rf build
    mkdir -p build
    cd build
    if cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS='-O3 -flto -funroll-loops' > /dev/null 2>&1 && \
       make ${version}_benchmark -j$(sysctl -n hw.ncpu) > /dev/null 2>&1; then
        echo "  ✅ Built successfully"
    else
        echo "  ⚠️  Failed to build $version benchmark"
        FAILED+=("$version (build failed)")
        cd "$SCRIPT_DIR"
        continue
    fi
    
    # Run benchmark
    cd "$SCRIPT_DIR/$VERSION_DIR"
    BENCHMARK_BIN_LOCAL="build/${version}_benchmark"
    if [ -f "$BENCHMARK_BIN_LOCAL" ] && [ -x "$BENCHMARK_BIN_LOCAL" ]; then
        echo "  Starting benchmark execution..."
        if ./"$BENCHMARK_BIN_LOCAL" "$NUM_ORDERS" 2>&1 | tee /tmp/${version}_mac_benchmark.log; then
            # Copy report if generated
            if [ -f "BENCHMARK_REPORT.md" ]; then
                cp BENCHMARK_REPORT.md "$SCRIPT_DIR/$REPORTS_DIR/${version}_BENCHMARK_REPORT.md"
                echo ""
                echo "  ✅ $version benchmark completed successfully"
                echo "  Report saved to: $SCRIPT_DIR/$REPORTS_DIR/${version}_BENCHMARK_REPORT.md"
                SUCCESSFUL+=("$version")
            else
                echo "  ⚠️  Benchmark ran but no report generated"
                FAILED+=("$version (no report)")
            fi
        else
            echo "  ⚠️  Benchmark execution failed"
            cat /tmp/${version}_mac_benchmark.log | tail -10
            FAILED+=("$version (execution failed)")
        fi
    else
        echo "  ⚠️  Binary not found or not executable: $BENCHMARK_BIN_LOCAL (pwd: $(pwd))"
        ls -la build/ 2>/dev/null | head -5
        FAILED+=("$version (no binary)")
    fi
    cd "$SCRIPT_DIR"
    echo ""
done

# Summary
echo "=========================================="
echo "Mac Benchmark Summary"
echo "=========================================="
echo "Successful: ${#SUCCESSFUL[@]}"
for v in "${SUCCESSFUL[@]}"; do
    echo "  ✅ $v"
done

echo ""
echo "Failed: ${#FAILED[@]}"
for v in "${FAILED[@]}"; do
    echo "  ❌ $v"
done

echo ""
echo "Reports saved to: $REPORTS_DIR"
echo ""

if [ ${#FAILED[@]} -eq 0 ]; then
    echo "✅ All Mac benchmarks completed successfully!"
    exit 0
else
    echo "⚠️  Some Mac benchmarks failed"
    exit 1
fi

