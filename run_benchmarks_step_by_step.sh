#!/bin/bash
# Step-by-step benchmark runner with real-time output

NUM_ORDERS=${1:-50000}
REPORTS_DIR="./benchmark_reports"

echo "=========================================="
echo "Step-by-Step Docker Benchmark Runner"
echo "=========================================="
echo ""

# Step 1: Build image
echo "Step 1/3: Building Docker image..."
docker compose -f docker-compose.benchmark.yml build benchmark-runner 2>&1 | \
    grep -E "Building|Built|ERROR|error|DONE" | tail -20

echo ""
echo "Step 2/3: Running benchmarks (real-time output)..."
echo "=========================================="

# Step 2: Run benchmarks with real-time output
docker compose -f docker-compose.benchmark.yml run --rm benchmark-runner \
    bash -c "
    set -x
    export NUM_ORDERS=$NUM_ORDERS
    /app/run_all_benchmarks.sh \$NUM_ORDERS
" 2>&1 | tee docker_benchmark_output.log

echo ""
echo "Step 3/3: Generating analysis report..."
docker compose -f docker-compose.benchmark.yml run --rm benchmark-runner \
    /app/generate_benchmark_report.sh 2>&1 | tail -20

echo ""
echo "✅ Complete! Reports in: $REPORTS_DIR"
ls -lh "$REPORTS_DIR"/*.md 2>/dev/null | head -10
