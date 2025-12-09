#!/bin/bash

echo "=========================================="
echo "代码与文档一致性检查工具"
echo "=========================================="
echo ""

PROJECT_ROOT="/Users/lan/Downloads/perpetual_exchange"
ISSUES=0

# 检查1: 重复定义
echo "1. 检查重复定义..."
echo ""

check_duplicate_class() {
    local file=$1
    local class_name=$2
    local count=$(grep -c "^class $class_name\|^struct $class_name" "$file" 2>/dev/null || echo "0")
    if [ "$count" -gt 1 ]; then
        echo "  ❌ $file: $class_name 重复定义 ($count 次)"
        ((ISSUES++))
        return 1
    fi
    return 0
}

check_duplicate_class "$PROJECT_ROOT/include/core/matching_engine_event_sourcing.h" "MatchingEngineEventSourcing"
check_duplicate_class "$PROJECT_ROOT/include/core/position_manager.h" "PositionManager"
check_duplicate_class "$PROJECT_ROOT/include/core/funding_rate_manager.h" "FundingRateManager"
check_duplicate_class "$PROJECT_ROOT/include/core/liquidation_engine.h" "LiquidationEngine"

# 检查2: 文档中的重复内容
echo ""
echo "2. 检查文档重复内容..."
echo ""

check_duplicate_section() {
    local file=$1
    local section=$2
    local count=$(grep -c "$section" "$file" 2>/dev/null || echo "0")
    if [ "$count" -gt 1 ]; then
        echo "  ❌ $file: '$section' 重复出现 ($count 次)"
        ((ISSUES++))
        return 1
    fi
    return 0
}

check_duplicate_section "$PROJECT_ROOT/ARCHITECTURE.md" "^## 概述"
check_duplicate_section "$PROJECT_ROOT/ARCHITECTURE.md" "^### 1. 订单簿"

# 检查3: API方法名一致性
echo ""
echo "3. 检查API方法名一致性..."
echo ""

# 检查文档中提到的API是否在代码中存在
check_api_in_code() {
    local api_name=$1
    local file=$2
    if grep -q "$api_name" "$file" 2>/dev/null; then
        echo "  ✅ $api_name 存在于代码中"
    else
        echo "  ⚠️  $api_name 在文档中提到但代码中不存在"
        ((ISSUES++))
    fi
}

echo "检查 MatchingEngineEventSourcing API..."
check_api_in_code "process_order_es" "$PROJECT_ROOT/include/core/matching_engine_event_sourcing.h"
check_api_in_code "cancel_order_es" "$PROJECT_ROOT/include/core/matching_engine_event_sourcing.h"
check_api_in_code "replay_events" "$PROJECT_ROOT/include/core/matching_engine_event_sourcing.h"

echo ""
echo "=========================================="
echo "检查完成，发现 $ISSUES 个问题"
echo "=========================================="
