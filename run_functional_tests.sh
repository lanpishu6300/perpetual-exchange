#!/bin/bash

# 运行非共识版本的功能测试脚本

set -e

cd "$(dirname "$0")"

echo "========================================"
echo "非共识版本功能测试套件"
echo "========================================"
echo ""

# 检查可执行文件
TESTS=(
    "build/test_comprehensive_functional"
    "build/test_order_flow"
    "build/test_api_integration"
    "build/test_consensus"
)

MISSING_TESTS=()
for test in "${TESTS[@]}"; do
    if [ ! -f "$test" ]; then
        MISSING_TESTS+=("$test")
    fi
done

if [ ${#MISSING_TESTS[@]} -gt 0 ]; then
    echo "❌ 错误: 以下测试程序未找到:"
    for test in "${MISSING_TESTS[@]}"; do
        echo "   $test"
    done
    echo ""
    echo "请先编译测试程序:"
    echo "  cd build && cmake .. && make"
    exit 1
fi

# 创建测试结果目录
RESULTS_DIR="./test_results_$(date +%Y%m%d_%H%M%S)"
mkdir -p "$RESULTS_DIR"

echo "测试结果将保存到: $RESULTS_DIR"
echo ""

# 运行测试并保存结果
PASSED=0
FAILED=0
TOTAL=0

for test in "${TESTS[@]}"; do
    TEST_NAME=$(basename "$test")
    echo "----------------------------------------"
    echo "运行测试: $TEST_NAME"
    echo "----------------------------------------"
    
    TOTAL=$((TOTAL + 1))
    
    if ./"$test" > "$RESULTS_DIR/${TEST_NAME}.log" 2>&1; then
        echo "✅ $TEST_NAME - 通过"
        PASSED=$((PASSED + 1))
    else
        echo "❌ $TEST_NAME - 失败"
        FAILED=$((FAILED + 1))
        echo "   查看日志: $RESULTS_DIR/${TEST_NAME}.log"
    fi
    echo ""
done

# 汇总结果
echo "========================================"
echo "测试结果汇总"
echo "========================================"
echo "总测试数: $TOTAL"
echo "通过: $PASSED"
echo "失败: $FAILED"
echo ""
echo "详细结果保存在: $RESULTS_DIR"
echo "========================================"

if [ $FAILED -eq 0 ]; then
    echo "✅ 所有测试通过！"
    exit 0
else
    echo "❌ 有 $FAILED 个测试失败"
    exit 1
fi


# 运行非共识版本的功能测试脚本

set -e

cd "$(dirname "$0")"

echo "========================================"
echo "非共识版本功能测试套件"
echo "========================================"
echo ""

# 检查可执行文件
TESTS=(
    "build/test_comprehensive_functional"
    "build/test_order_flow"
    "build/test_api_integration"
    "build/test_consensus"
)

MISSING_TESTS=()
for test in "${TESTS[@]}"; do
    if [ ! -f "$test" ]; then
        MISSING_TESTS+=("$test")
    fi
done

if [ ${#MISSING_TESTS[@]} -gt 0 ]; then
    echo "❌ 错误: 以下测试程序未找到:"
    for test in "${MISSING_TESTS[@]}"; do
        echo "   $test"
    done
    echo ""
    echo "请先编译测试程序:"
    echo "  cd build && cmake .. && make"
    exit 1
fi

# 创建测试结果目录
RESULTS_DIR="./test_results_$(date +%Y%m%d_%H%M%S)"
mkdir -p "$RESULTS_DIR"

echo "测试结果将保存到: $RESULTS_DIR"
echo ""

# 运行测试并保存结果
PASSED=0
FAILED=0
TOTAL=0

for test in "${TESTS[@]}"; do
    TEST_NAME=$(basename "$test")
    echo "----------------------------------------"
    echo "运行测试: $TEST_NAME"
    echo "----------------------------------------"
    
    TOTAL=$((TOTAL + 1))
    
    if ./"$test" > "$RESULTS_DIR/${TEST_NAME}.log" 2>&1; then
        echo "✅ $TEST_NAME - 通过"
        PASSED=$((PASSED + 1))
    else
        echo "❌ $TEST_NAME - 失败"
        FAILED=$((FAILED + 1))
        echo "   查看日志: $RESULTS_DIR/${TEST_NAME}.log"
    fi
    echo ""
done

# 汇总结果
echo "========================================"
echo "测试结果汇总"
echo "========================================"
echo "总测试数: $TOTAL"
echo "通过: $PASSED"
echo "失败: $FAILED"
echo ""
echo "详细结果保存在: $RESULTS_DIR"
echo "========================================"

if [ $FAILED -eq 0 ]; then
    echo "✅ 所有测试通过！"
    exit 0
else
    echo "❌ 有 $FAILED 个测试失败"
    exit 1
fi

