#!/bin/bash

echo "=== 分片架构性能验证 ==="
echo ""

# 测试1: WAL禁用模式（最高性能）
echo "测试1: WAL禁用模式（最高性能）"
echo "----------------------------------------"
cd "$(dirname "$0")"
./build/production_safe_optimized_sharded_benchmark 2>&1 | grep -E "(吞吐量|延迟|P50|P90|P99|最终结果|===)" | head -30

echo ""
echo "测试完成！"



echo "=== 分片架构性能验证 ==="
echo ""

# 测试1: WAL禁用模式（最高性能）
echo "测试1: WAL禁用模式（最高性能）"
echo "----------------------------------------"
cd "$(dirname "$0")"
./build/production_safe_optimized_sharded_benchmark 2>&1 | grep -E "(吞吐量|延迟|P50|P90|P99|最终结果|===)" | head -30

echo ""
echo "测试完成！"


