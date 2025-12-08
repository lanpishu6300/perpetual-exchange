#!/bin/bash

# 运行所有性能测试并生成对比报告

set -e

echo "=========================================="
echo "运行所有性能基准测试"
echo "=========================================="
echo ""

cd build

echo "[1/4] 原始版本基准测试..."
./quick_benchmark > ../reports/original_benchmark.txt 2>&1 || echo "跳过"

echo ""
echo "[2/4] 优化版本对比测试..."
./quick_comparison > ../reports/optimized_comparison.txt 2>&1 || echo "跳过"

echo ""
echo "[3/4] 持久化性能测试..."
./persistence_benchmark > ../reports/persistence_benchmark.txt 2>&1 || echo "跳过"

echo ""
echo "[4/4] 完整性能对比测试..."
./comprehensive_performance_comparison > ../reports/comprehensive_comparison.txt 2>&1 || echo "跳过"

echo ""
echo "=========================================="
echo "所有测试完成！"
echo "=========================================="
echo ""
echo "报告位置:"
echo "  - reports/original_benchmark.txt"
echo "  - reports/optimized_comparison.txt"
echo "  - reports/persistence_benchmark.txt"
echo "  - reports/comprehensive_comparison.txt"
echo "  - comprehensive_performance_report.txt"
echo ""



