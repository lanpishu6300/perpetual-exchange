#!/bin/bash

echo "=========================================="
echo "性能优化设计文档与代码实现一致性检查"
echo "=========================================="
echo ""

PROJECT_ROOT="/Users/lan/Downloads/perpetual_exchange"
ISSUES=0

echo "1. 检查 SIMD 优化实现..."
echo ""

# 检查 SIMD 实现
check_simd() {
    if grep -q "AVX-512\|__m512\|_mm512" "$PROJECT_ROOT/include/core/simd_utils.h" 2>/dev/null; then
        echo "  ✅ AVX-512 支持"
    elif grep -q "AVX2\|__m256\|_mm256" "$PROJECT_ROOT/include/core/simd_utils.h" 2>/dev/null; then
        echo "  ⚠️  文档提到 AVX-512，但代码实现使用 AVX2"
        ((ISSUES++))
    else
        echo "  ❌ 文档提到 SIMD，但代码中未找到实现"
        ((ISSUES++))
    fi
}

check_simd

echo ""
echo "2. 检查 Lock-Free 实现..."
echo ""

check_lockfree() {
    if [ -f "$PROJECT_ROOT/include/core/lockfree_queue.h" ]; then
        if grep -q "LockFreeSPSCQueue\|Lock-Free" "$PROJECT_ROOT/include/core/lockfree_queue.h" 2>/dev/null; then
            echo "  ✅ Lock-Free SPSC Queue 已实现"
        else
            echo "  ⚠️  Lock-Free Queue 文件存在但实现可能不完整"
            ((ISSUES++))
        fi
    else
        echo "  ❌ 文档提到 Lock-Free Queue，但文件不存在"
        ((ISSUES++))
    fi
}

check_lockfree

echo ""
echo "3. 检查内存池实现..."
echo ""

check_memory_pool() {
    if [ -f "$PROJECT_ROOT/include/core/memory_pool.h" ]; then
        if grep -q "MemoryPool\|lock-free\|Lock-Free" "$PROJECT_ROOT/include/core/memory_pool.h" 2>/dev/null; then
            echo "  ✅ 内存池已实现"
        else
            echo "  ⚠️  内存池文件存在但可能不是 Lock-Free"
            ((ISSUES++))
        fi
    else
        echo "  ❌ 文档提到内存池，但文件不存在"
        ((ISSUES++))
    fi
}

check_memory_pool

echo ""
echo "4. 检查异步持久化实现..."
echo ""

check_async_persistence() {
    if [ -f "$PROJECT_ROOT/include/core/persistence_async.h" ]; then
        if grep -q "AsyncPersistenceManager\|LockFreeQueue" "$PROJECT_ROOT/include/core/persistence_async.h" 2>/dev/null; then
            echo "  ✅ 异步持久化已实现"
        else
            echo "  ⚠️  异步持久化文件存在但可能未使用 Lock-Free"
            ((ISSUES++))
        fi
    else
        echo "  ❌ 文档提到异步持久化，但文件不存在"
        ((ISSUES++))
    fi
}

check_async_persistence

echo ""
echo "5. 检查 NUMA 优化实现..."
echo ""

check_numa() {
    if [ -f "$PROJECT_ROOT/include/core/numa_utils.h" ]; then
        if grep -q "NUMA\|numa\|bind_thread" "$PROJECT_ROOT/include/core/numa_utils.h" 2>/dev/null; then
            echo "  ✅ NUMA 工具已实现"
        else
            echo "  ⚠️  NUMA 工具文件存在但实现可能不完整"
            ((ISSUES++))
        fi
    else
        echo "  ❌ 文档提到 NUMA 优化，但文件不存在"
        ((ISSUES++))
    fi
}

check_numa

echo ""
echo "6. 检查 ART 树实现..."
echo ""

check_art() {
    if [ -f "$PROJECT_ROOT/include/core/art_tree.h" ]; then
        echo "  ✅ ART 树已实现"
    else
        echo "  ❌ 文档提到 ART 树，但文件不存在"
        ((ISSUES++))
    fi
}

check_art

echo ""
echo "=========================================="
echo "检查完成，发现 $ISSUES 个潜在不一致问题"
echo "=========================================="
