#!/bin/bash

# Script to generate comprehensive benchmark report from all version reports
# Usage: ./generate_benchmark_report.sh

set -e

REPORTS_DIR="/app/reports"
OUTPUT_REPORT="$REPORTS_DIR/COMPREHENSIVE_BENCHMARK_REPORT.md"

echo "=========================================="
echo "Generating Comprehensive Benchmark Report"
echo "=========================================="

cat > "$OUTPUT_REPORT" << 'EOF'
# Comprehensive Benchmark Report - All Versions

## Overview

This report summarizes the performance benchmarks for all versions of the Perpetual Exchange matching engine.

**Generated**: $(date)
**Test Environment**: Docker Container
**Platform**: Linux/amd64

---

EOF

# Add summary table
cat >> "$OUTPUT_REPORT" << 'EOF'
## Summary Table

| Version | Throughput | Avg Latency | P50 Latency | P90 Latency | P99 Latency | Status |
|---------|------------|-------------|-------------|-------------|-------------|--------|
EOF

# Process each version report
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
)

for version in "${VERSIONS[@]}"; do
    REPORT_FILE="$REPORTS_DIR/${version}_BENCHMARK_REPORT.md"
    
    if [ -f "$REPORT_FILE" ]; then
        # Extract metrics using grep and awk
        THROUGHPUT=$(grep -i "Throughput" "$REPORT_FILE" | grep -oE "[0-9]+\.[0-9]+" | head -1 || echo "N/A")
        AVG_LATENCY=$(grep -i "Average" "$REPORT_FILE" | grep -oE "[0-9]+\.[0-9]+" | head -1 || echo "N/A")
        P50_LATENCY=$(grep -i "P50" "$REPORT_FILE" | grep -oE "[0-9]+\.[0-9]+" | head -1 || echo "N/A")
        P90_LATENCY=$(grep -i "P90" "$REPORT_FILE" | grep -oE "[0-9]+\.[0-9]+" | head -1 || echo "N/A")
        P99_LATENCY=$(grep -i "P99" "$REPORT_FILE" | grep -oE "[0-9]+\.[0-9]+" | head -1 || echo "N/A")
        
        echo "| $version | $THROUGHPUT K orders/sec | $AVG_LATENCY μs | $P50_LATENCY μs | $P90_LATENCY μs | $P99_LATENCY μs | ✅ |" >> "$OUTPUT_REPORT"
    else
        echo "| $version | N/A | N/A | N/A | N/A | N/A | ❌ |" >> "$OUTPUT_REPORT"
    fi
done

cat >> "$OUTPUT_REPORT" << 'EOF'

---

## Detailed Reports

EOF

# Append each version's detailed report
for version in "${VERSIONS[@]}"; do
    REPORT_FILE="$REPORTS_DIR/${version}_BENCHMARK_REPORT.md"
    
    if [ -f "$REPORT_FILE" ]; then
        echo "### $version" >> "$OUTPUT_REPORT"
        echo "" >> "$OUTPUT_REPORT"
        echo "\`\`\`" >> "$OUTPUT_REPORT"
        cat "$REPORT_FILE" >> "$OUTPUT_REPORT"
        echo "\`\`\`" >> "$OUTPUT_REPORT"
        echo "" >> "$OUTPUT_REPORT"
        echo "---" >> "$OUTPUT_REPORT"
        echo "" >> "$OUTPUT_REPORT"
    fi
done

echo "✅ Comprehensive report generated: $OUTPUT_REPORT"
cat "$OUTPUT_REPORT"



