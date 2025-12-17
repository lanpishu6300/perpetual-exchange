#!/bin/bash

echo "=========================================="
echo "Wiki 文档与代码实现一致性检查"
echo "=========================================="
echo ""

PROJECT_ROOT="/Users/lan/Downloads/perpetual_exchange"
WIKI_DIR="/tmp/matching-engine-wiki"
ISSUES=0

echo "1. 检查 DeterministicCalculator API..."
echo ""

# 检查文档中提到的 API
check_api() {
    local doc_name=$1
    local actual_name=$2
    local file=$3
    
    if grep -q "$actual_name" "$file" 2>/dev/null; then
        if [ "$doc_name" != "$actual_name" ]; then
            echo "  ⚠️  文档使用: $doc_name"
            echo "     实际代码: $actual_name"
            echo "     文件: $file"
            ((ISSUES++))
        else
            echo "  ✅ $doc_name - 一致"
        fi
    else
        echo "  ❌ $doc_name - 代码中不存在"
        ((ISSUES++))
    fi
}

# 检查 DeterministicCalculator 方法
check_api "comparePrice" "compare_prices" "$PROJECT_ROOT/include/core/deterministic_calculator.h"
check_api "calculateMatchPrice" "calculate_match_price" "$PROJECT_ROOT/include/core/deterministic_calculator.h"

echo ""
echo "2. 检查 MatchingEngineEventSourcing API..."
echo ""

check_api "process_order_es" "process_order_es" "$PROJECT_ROOT/include/core/matching_engine_event_sourcing.h"
check_api "cancel_order_es" "cancel_order_es" "$PROJECT_ROOT/include/core/matching_engine_event_sourcing.h"
check_api "replay_events" "replay_events" "$PROJECT_ROOT/include/core/matching_engine_event_sourcing.h"

echo ""
echo "3. 检查 EventStore API..."
echo ""

check_api "get_latest_sequence" "get_latest_sequence" "$PROJECT_ROOT/include/core/event_sourcing.h"
check_api "replay_events" "replay_events" "$PROJECT_ROOT/include/core/event_sourcing.h"

echo ""
echo "=========================================="
echo "检查完成，发现 $ISSUES 个不一致问题"
echo "=========================================="
