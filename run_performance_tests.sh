#!/bin/bash

# 运行非共识版本的性能测试脚本

set -e

cd "$(dirname "$0")"

echo "========================================"
echo "非共识版本性能测试"
echo "========================================"
echo ""

# 检查可执行文件
if [ ! -f "./build/test_optimized_v3_safe" ]; then
    echo "❌ 错误: test_optimized_v3_safe 未找到"
    echo "请先编译: cd build && cmake .. && make test_optimized_v3_safe"
    exit 1
fi

# 创建测试结果目录
RESULTS_DIR="./perf_results_$(date +%Y%m%d_%H%M%S)"
mkdir -p "$RESULTS_DIR"

echo "性能测试结果将保存到: $RESULTS_DIR"
echo ""

# 测试配置
CONFIGS=(
    "2 3 2000"    # 2线程, 3秒, 2000订单
    "4 5 5000"    # 4线程, 5秒, 5000订单
    "2 5 10000"   # 2线程, 5秒, 10000订单
)

CONFIG_NAMES=(
    "light"       # 轻量级
    "medium"      # 中等负载
    "heavy"       # 高负载
)

for i in "${!CONFIGS[@]}"; do
    CONFIG="${CONFIGS[$i]}"
    NAME="${CONFIG_NAMES[$i]}"
    
    echo "----------------------------------------"
    echo "运行性能测试: $NAME (配置: $CONFIG)"
    echo "----------------------------------------"
    
    OUTPUT_FILE="$RESULTS_DIR/perf_${NAME}.log"
    
    echo "开始时间: $(date)" | tee "$OUTPUT_FILE"
    echo "配置: $CONFIG" | tee -a "$OUTPUT_FILE"
    echo "" | tee -a "$OUTPUT_FILE"
    
    if ./build/test_optimized_v3_safe $CONFIG 2>&1 | tee -a "$OUTPUT_FILE"; then
        echo "" | tee -a "$OUTPUT_FILE"
        echo "结束时间: $(date)" | tee -a "$OUTPUT_FILE"
        echo "✅ $NAME 测试完成"
    else
        echo "❌ $NAME 测试失败"
        echo "   查看日志: $OUTPUT_FILE"
    fi
    echo ""
done

# 生成汇总报告
REPORT_FILE="$RESULTS_DIR/performance_summary.md"
cat > "$REPORT_FILE" << EOF
# 性能测试汇总报告

生成时间: $(date)

## 测试配置

| 测试名称 | 线程数 | 持续时间(秒) | 订单数 |
|---------|--------|-------------|--------|
| light   | 2      | 3           | 2000   |
| medium  | 4      | 5           | 5000   |
| heavy   | 2      | 5           | 10000  |

## 测试结果

详细结果请查看各个测试的日志文件。

EOF

echo "========================================"
echo "性能测试完成"
echo "========================================"
echo "结果保存在: $RESULTS_DIR"
echo "汇总报告: $REPORT_FILE"
echo "========================================"


# 运行非共识版本的性能测试脚本

set -e

cd "$(dirname "$0")"

echo "========================================"
echo "非共识版本性能测试"
echo "========================================"
echo ""

# 检查可执行文件
if [ ! -f "./build/test_optimized_v3_safe" ]; then
    echo "❌ 错误: test_optimized_v3_safe 未找到"
    echo "请先编译: cd build && cmake .. && make test_optimized_v3_safe"
    exit 1
fi

# 创建测试结果目录
RESULTS_DIR="./perf_results_$(date +%Y%m%d_%H%M%S)"
mkdir -p "$RESULTS_DIR"

echo "性能测试结果将保存到: $RESULTS_DIR"
echo ""

# 测试配置
CONFIGS=(
    "2 3 2000"    # 2线程, 3秒, 2000订单
    "4 5 5000"    # 4线程, 5秒, 5000订单
    "2 5 10000"   # 2线程, 5秒, 10000订单
)

CONFIG_NAMES=(
    "light"       # 轻量级
    "medium"      # 中等负载
    "heavy"       # 高负载
)

for i in "${!CONFIGS[@]}"; do
    CONFIG="${CONFIGS[$i]}"
    NAME="${CONFIG_NAMES[$i]}"
    
    echo "----------------------------------------"
    echo "运行性能测试: $NAME (配置: $CONFIG)"
    echo "----------------------------------------"
    
    OUTPUT_FILE="$RESULTS_DIR/perf_${NAME}.log"
    
    echo "开始时间: $(date)" | tee "$OUTPUT_FILE"
    echo "配置: $CONFIG" | tee -a "$OUTPUT_FILE"
    echo "" | tee -a "$OUTPUT_FILE"
    
    if ./build/test_optimized_v3_safe $CONFIG 2>&1 | tee -a "$OUTPUT_FILE"; then
        echo "" | tee -a "$OUTPUT_FILE"
        echo "结束时间: $(date)" | tee -a "$OUTPUT_FILE"
        echo "✅ $NAME 测试完成"
    else
        echo "❌ $NAME 测试失败"
        echo "   查看日志: $OUTPUT_FILE"
    fi
    echo ""
done

# 生成汇总报告
REPORT_FILE="$RESULTS_DIR/performance_summary.md"
cat > "$REPORT_FILE" << EOF
# 性能测试汇总报告

生成时间: $(date)

## 测试配置

| 测试名称 | 线程数 | 持续时间(秒) | 订单数 |
|---------|--------|-------------|--------|
| light   | 2      | 3           | 2000   |
| medium  | 4      | 5           | 5000   |
| heavy   | 2      | 5           | 10000  |

## 测试结果

详细结果请查看各个测试的日志文件。

EOF

echo "========================================"
echo "性能测试完成"
echo "========================================"
echo "结果保存在: $RESULTS_DIR"
echo "汇总报告: $REPORT_FILE"
echo "========================================"

