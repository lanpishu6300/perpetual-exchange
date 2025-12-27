#!/bin/bash

# 分别运行交易分片和撮合分片的独立压测

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"

# 颜色输出
GREEN='\033[0;32m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

echo -e "${CYAN}=========================================="
echo "分片独立压测 - Separate Shard Benchmarks"
echo "==========================================${NC}"
echo ""

# 创建必要的目录
mkdir -p "$SCRIPT_DIR/data/wal"

cd "$BUILD_DIR"

# 参数
NUM_ORDERS=${1:-50000}
NUM_SHARDS=${2:-$(sysctl -n hw.ncpu 2>/dev/null || nproc 2>/dev/null || echo 4)}

echo -e "${BLUE}测试参数:${NC}"
echo "  订单数: $NUM_ORDERS"
echo "  分片数: $NUM_SHARDS"
echo ""

# 1. 交易分片压测
echo -e "${GREEN}=========================================="
echo "1. 交易分片压测 (Trading Shard)"
echo "==========================================${NC}"
echo ""

./production_safe_optimized_benchmark_trading_shard "$NUM_ORDERS" "$NUM_SHARDS"

echo ""
echo ""

# 2. 撮合分片压测（无WAL）
echo -e "${GREEN}=========================================="
echo "2. 撮合分片压测 (Matching Shard - No WAL)"
echo "==========================================${NC}"
echo ""

./production_safe_optimized_benchmark_matching_shard "$NUM_ORDERS" "$NUM_SHARDS" 0

echo ""
echo ""

# 3. 撮合分片压测（有WAL）
echo -e "${GREEN}=========================================="
echo "3. 撮合分片压测 (Matching Shard - With WAL)"
echo "==========================================${NC}"
echo ""

./production_safe_optimized_benchmark_matching_shard "$NUM_ORDERS" "$NUM_SHARDS" 1

echo ""
echo -e "${CYAN}=========================================="
echo "所有压测完成"
echo "==========================================${NC}"

