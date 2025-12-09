#!/bin/bash
# 运行所有测试套件的脚本

cd "$(dirname "$0")"
BUILD_DIR="./build"

echo "=========================================="
echo "开始运行所有测试套件"
echo "=========================================="
echo ""

# 测试1: 综合功能测试
echo "[1/4] 运行综合功能测试..."
if [ -f "$BUILD_DIR/test_comprehensive_functional" ]; then
    $BUILD_DIR/test_comprehensive_functional 2>&1 | tee /tmp/test_comprehensive_functional.log
    COMPREHENSIVE_EXIT=$?
    echo "综合功能测试退出码: $COMPREHENSIVE_EXIT"
else
    echo "错误: test_comprehensive_functional 不存在"
    COMPREHENSIVE_EXIT=1
fi
echo ""

# 测试2: 订单流程测试
echo "[2/4] 运行订单流程测试..."
if [ -f "$BUILD_DIR/test_order_flow" ]; then
    $BUILD_DIR/test_order_flow 2>&1 | tee /tmp/test_order_flow.log
    ORDER_FLOW_EXIT=$?
    echo "订单流程测试退出码: $ORDER_FLOW_EXIT"
else
    echo "错误: test_order_flow 不存在"
    ORDER_FLOW_EXIT=1
fi
echo ""

# 测试3: API集成测试
echo "[3/4] 运行API集成测试..."
if [ -f "$BUILD_DIR/test_api_integration" ]; then
    $BUILD_DIR/test_api_integration 2>&1 | tee /tmp/test_api_integration.log
    API_EXIT=$?
    echo "API集成测试退出码: $API_EXIT"
else
    echo "错误: test_api_integration 不存在"
    API_EXIT=1
fi
echo ""

# 测试4: 共识模块测试
echo "[4/4] 运行共识模块测试..."
if [ -f "$BUILD_DIR/test_consensus" ]; then
    $BUILD_DIR/test_consensus 2>&1 | tee /tmp/test_consensus.log
    CONSENSUS_EXIT=$?
    echo "共识模块测试退出码: $CONSENSUS_EXIT"
else
    echo "错误: test_consensus 不存在"
    CONSENSUS_EXIT=1
fi
echo ""

# 汇总结果
echo "=========================================="
echo "测试结果汇总"
echo "=========================================="
echo "综合功能测试: $([ $COMPREHENSIVE_EXIT -eq 0 ] && echo '通过' || echo '失败')"
echo "订单流程测试: $([ $ORDER_FLOW_EXIT -eq 0 ] && echo '通过' || echo '失败')"
echo "API集成测试: $([ $API_EXIT -eq 0 ] && echo '通过' || echo '失败')"
echo "共识模块测试: $([ $CONSENSUS_EXIT -eq 0 ] && echo '通过' || echo '失败')"
echo ""

TOTAL_EXIT=$((COMPREHENSIVE_EXIT + ORDER_FLOW_EXIT + API_EXIT + CONSENSUS_EXIT))
if [ $TOTAL_EXIT -eq 0 ]; then
    echo "所有测试通过！"
    exit 0
else
    echo "部分测试失败，总退出码: $TOTAL_EXIT"
    exit 1
fi

# 运行所有测试套件的脚本

cd "$(dirname "$0")"
BUILD_DIR="./build"

echo "=========================================="
echo "开始运行所有测试套件"
echo "=========================================="
echo ""

# 测试1: 综合功能测试
echo "[1/4] 运行综合功能测试..."
if [ -f "$BUILD_DIR/test_comprehensive_functional" ]; then
    $BUILD_DIR/test_comprehensive_functional 2>&1 | tee /tmp/test_comprehensive_functional.log
    COMPREHENSIVE_EXIT=$?
    echo "综合功能测试退出码: $COMPREHENSIVE_EXIT"
else
    echo "错误: test_comprehensive_functional 不存在"
    COMPREHENSIVE_EXIT=1
fi
echo ""

# 测试2: 订单流程测试
echo "[2/4] 运行订单流程测试..."
if [ -f "$BUILD_DIR/test_order_flow" ]; then
    $BUILD_DIR/test_order_flow 2>&1 | tee /tmp/test_order_flow.log
    ORDER_FLOW_EXIT=$?
    echo "订单流程测试退出码: $ORDER_FLOW_EXIT"
else
    echo "错误: test_order_flow 不存在"
    ORDER_FLOW_EXIT=1
fi
echo ""

# 测试3: API集成测试
echo "[3/4] 运行API集成测试..."
if [ -f "$BUILD_DIR/test_api_integration" ]; then
    $BUILD_DIR/test_api_integration 2>&1 | tee /tmp/test_api_integration.log
    API_EXIT=$?
    echo "API集成测试退出码: $API_EXIT"
else
    echo "错误: test_api_integration 不存在"
    API_EXIT=1
fi
echo ""

# 测试4: 共识模块测试
echo "[4/4] 运行共识模块测试..."
if [ -f "$BUILD_DIR/test_consensus" ]; then
    $BUILD_DIR/test_consensus 2>&1 | tee /tmp/test_consensus.log
    CONSENSUS_EXIT=$?
    echo "共识模块测试退出码: $CONSENSUS_EXIT"
else
    echo "错误: test_consensus 不存在"
    CONSENSUS_EXIT=1
fi
echo ""

# 汇总结果
echo "=========================================="
echo "测试结果汇总"
echo "=========================================="
echo "综合功能测试: $([ $COMPREHENSIVE_EXIT -eq 0 ] && echo '通过' || echo '失败')"
echo "订单流程测试: $([ $ORDER_FLOW_EXIT -eq 0 ] && echo '通过' || echo '失败')"
echo "API集成测试: $([ $API_EXIT -eq 0 ] && echo '通过' || echo '失败')"
echo "共识模块测试: $([ $CONSENSUS_EXIT -eq 0 ] && echo '通过' || echo '失败')"
echo ""

TOTAL_EXIT=$((COMPREHENSIVE_EXIT + ORDER_FLOW_EXIT + API_EXIT + CONSENSUS_EXIT))
if [ $TOTAL_EXIT -eq 0 ]; then
    echo "所有测试通过！"
    exit 0
else
    echo "部分测试失败，总退出码: $TOTAL_EXIT"
    exit 1
fi

