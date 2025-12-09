#!/bin/bash

# Phase 1 优化性能测试脚本

set -e

cd "$(dirname "$0")"

echo "========================================"
echo "Phase 1 优化性能测试"
echo "========================================"
echo ""

# 检查可执行文件
if [ ! -f "./build/test_optimized_v3" ]; then
    echo "❌ 错误: test_optimized_v3 未找到，请先编译"
    exit 1
fi

# 测试配置
THREADS=4
DURATION=10  # 秒
ORDERS_PER_SEC=1000

if [ $# -ge 1 ]; then
    THREADS=$1
fi
if [ $# -ge 2 ]; then
    DURATION=$2
fi
if [ $# -ge 3 ]; then
    ORDERS_PER_SEC=$3
fi

echo "测试配置:"
echo "  线程数: $THREADS"
echo "  持续时间: ${DURATION}秒"
echo "  每线程订单速率: ${ORDERS_PER_SEC} orders/sec"
echo "  目标总TPS: $((THREADS * ORDERS_PER_SEC))"
echo ""

# 运行测试
echo "开始性能测试..."
echo ""

./build/test_optimized_v3 $THREADS $DURATION $ORDERS_PER_SEC

echo ""
echo "========================================"
echo "性能测试完成"
echo "========================================"


# Phase 1 优化性能测试脚本

set -e

cd "$(dirname "$0")"

echo "========================================"
echo "Phase 1 优化性能测试"
echo "========================================"
echo ""

# 检查可执行文件
if [ ! -f "./build/test_optimized_v3" ]; then
    echo "❌ 错误: test_optimized_v3 未找到，请先编译"
    exit 1
fi

# 测试配置
THREADS=4
DURATION=10  # 秒
ORDERS_PER_SEC=1000

if [ $# -ge 1 ]; then
    THREADS=$1
fi
if [ $# -ge 2 ]; then
    DURATION=$2
fi
if [ $# -ge 3 ]; then
    ORDERS_PER_SEC=$3
fi

echo "测试配置:"
echo "  线程数: $THREADS"
echo "  持续时间: ${DURATION}秒"
echo "  每线程订单速率: ${ORDERS_PER_SEC} orders/sec"
echo "  目标总TPS: $((THREADS * ORDERS_PER_SEC))"
echo ""

# 运行测试
echo "开始性能测试..."
echo ""

./build/test_optimized_v3 $THREADS $DURATION $ORDERS_PER_SEC

echo ""
echo "========================================"
echo "性能测试完成"
echo "========================================"

