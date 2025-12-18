#!/bin/bash

# Script to run benchmarks for all versions in Docker
# Usage: ./run_all_benchmarks.sh [num_orders]

set -e

NUM_ORDERS=${1:-50000}
REPORTS_DIR="/app/reports"
BENCHMARKS_DIR="/app/benchmarks"

echo "=========================================="
echo "Running Benchmarks for All Versions"
echo "=========================================="
echo "Number of orders: $NUM_ORDERS"
echo "Reports directory: $REPORTS_DIR"
echo ""

# Create reports directory
mkdir -p "$REPORTS_DIR"

# List of versions to benchmark
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

# Function to build benchmark if needed
build_benchmark() {
    local version=$1
    local VERSION_DIR="/app/versions/$version"
    local BUILD_DIR="$VERSION_DIR/build"
    
    echo "  Building $version benchmark..."
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
    
    if cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-O3 -flto -funroll-loops" > /dev/null 2>&1 && \
       make ${version}_benchmark -j$(nproc) > /dev/null 2>&1; then
        cd /app
        return 0
    else
        cd /app
        return 1
    fi
}

# Run benchmark for each version
for version in "${VERSIONS[@]}"; do
    echo "----------------------------------------"
    echo "Running benchmark for: $version"
    echo "----------------------------------------"
    
    BENCHMARK_BIN="/app/versions/$version/build/${version}_benchmark"
    REPORT_FILE="$REPORTS_DIR/${version}_BENCHMARK_REPORT.md"
    VERSION_DIR="/app/versions/$version"
    
    # Check if binary exists and is executable
    if [ ! -f "$BENCHMARK_BIN" ] || [ ! -x "$BENCHMARK_BIN" ]; then
        echo "  Binary not found or not executable, building..."
        if ! build_benchmark "$version"; then
            echo "  ⚠️  Failed to build $version benchmark"
            FAILED+=("$version (build failed)")
            continue
        fi
    fi
    
    # Verify binary is Linux executable
    if file "$BENCHMARK_BIN" 2>/dev/null | grep -q "ELF.*Linux"; then
        echo "  ✅ Found Linux binary: $BENCHMARK_BIN"
    else
        echo "  ⚠️  Binary format issue, rebuilding..."
        rm -rf "$VERSION_DIR/build"
        if ! build_benchmark "$version"; then
            echo "  ⚠️  Failed to rebuild $version benchmark"
            FAILED+=("$version (rebuild failed)")
            continue
        fi
    fi
    
    # Change to version directory
    if [ -d "$VERSION_DIR" ]; then
        cd "$VERSION_DIR"
    fi
    
    # Run benchmark with real-time output
    echo "  Starting benchmark execution..."
    echo "  ========================================"
    if "$BENCHMARK_BIN" "$NUM_ORDERS" 2>&1 | tee "$REPORT_FILE.tmp"; then
        mv "$REPORT_FILE.tmp" "$REPORT_FILE"
        echo ""
        echo "  ✅ $version benchmark completed successfully"
        SUCCESSFUL+=("$version")
        
        # Check if report was generated in current directory
        if [ -f "BENCHMARK_REPORT.md" ]; then
            cp BENCHMARK_REPORT.md "$REPORT_FILE"
            echo "  Report saved to: $REPORT_FILE"
        elif [ -f "$REPORT_FILE" ] && grep -q "Performance Benchmark Report\|压测报告" "$REPORT_FILE"; then
            echo "  Report saved to: $REPORT_FILE"
        else
            echo "  ⚠️  Report may not have been generated correctly"
        fi
    else
        if [ -f "$REPORT_FILE.tmp" ]; then
            mv "$REPORT_FILE.tmp" "$REPORT_FILE"
        fi
        echo ""
        echo "  ❌ $version benchmark failed"
        FAILED+=("$version")
    fi
    
    cd /app
    echo ""
done

# Summary
echo "=========================================="
echo "Benchmark Summary"
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
ls -lh "$REPORTS_DIR"/*.md 2>/dev/null || echo "No reports found"

echo ""
echo "✅ Benchmark run completed!"
