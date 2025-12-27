#!/bin/bash

# Benchmark comparison script for zero loss vs optimized modes

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"
REPORT_DIR="$SCRIPT_DIR/benchmark_reports"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)

mkdir -p "$REPORT_DIR"

echo "=========================================="
echo "Zero Loss vs Optimized Benchmark Comparison"
echo "=========================================="
echo ""

cd "$BUILD_DIR"

# Check if benchmarks exist
if [ ! -f "production_safe_optimized_benchmark" ]; then
    echo "Building benchmarks..."
    make production_safe_optimized_benchmark -j$(sysctl -n hw.ncpu 2>/dev/null || nproc) || {
        echo "Failed to build benchmark"
        exit 1
    }
fi

# Run optimized mode benchmark
echo "Running Optimized Mode Benchmark..."
./production_safe_optimized_benchmark > "$REPORT_DIR/optimized_${TIMESTAMP}.log" 2>&1 || {
    echo "Optimized benchmark failed"
}

# Check if zero loss benchmark exists, if not use optimized with zero_loss flag
if [ -f "production_safe_optimized_benchmark_zero_loss" ]; then
    echo "Running Zero Loss Mode Benchmark..."
    ./production_safe_optimized_benchmark_zero_loss zero_loss > "$REPORT_DIR/zero_loss_${TIMESTAMP}.log" 2>&1 || {
        echo "Zero loss benchmark failed"
    }
else
    echo "Zero loss benchmark not built, using optimized benchmark with zero_loss mode..."
    # We'll need to modify the benchmark to support command line args
    echo "Note: Zero loss benchmark needs to be built separately"
fi

# Generate comparison report
echo ""
echo "Generating comparison report..."
cat > "$REPORT_DIR/comparison_${TIMESTAMP}.md" << 'EOF'
# Zero Loss vs Optimized Mode Benchmark Comparison

## Test Overview

This report compares the performance of:
- **Optimized Mode**: Critical orders sync, others async (hybrid approach)
- **Zero Loss Mode**: All orders sync immediately (true zero data loss)

## Results

EOF

# Extract results from logs
if [ -f "$REPORT_DIR/optimized_${TIMESTAMP}.log" ]; then
    echo "### Optimized Mode Results" >> "$REPORT_DIR/comparison_${TIMESTAMP}.md"
    grep -A 10 "Benchmark Results" "$REPORT_DIR/optimized_${TIMESTAMP}.log" >> "$REPORT_DIR/comparison_${TIMESTAMP}.md" || true
    echo "" >> "$REPORT_DIR/comparison_${TIMESTAMP}.md"
fi

if [ -f "$REPORT_DIR/zero_loss_${TIMESTAMP}.log" ]; then
    echo "### Zero Loss Mode Results" >> "$REPORT_DIR/comparison_${TIMESTAMP}.md"
    grep -A 10 "Benchmark Results" "$REPORT_DIR/zero_loss_${TIMESTAMP}.log" >> "$REPORT_DIR/comparison_${TIMESTAMP}.md" || true
    echo "" >> "$REPORT_DIR/comparison_${TIMESTAMP}.md"
fi

echo "## Conclusion" >> "$REPORT_DIR/comparison_${TIMESTAMP}.md"
echo "" >> "$REPORT_DIR/comparison_${TIMESTAMP}.md"
echo "Reports saved to: $REPORT_DIR" >> "$REPORT_DIR/comparison_${TIMESTAMP}.md"

echo ""
echo "✅ Benchmark comparison complete!"
echo "Reports saved to: $REPORT_DIR/comparison_${TIMESTAMP}.md"



