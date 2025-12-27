#!/bin/bash

# 文档分类合并脚本
# 将所有文档按类别分类并合并

set -e

# Colors
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

echo "========================================"
echo "文档分类合并工具"
echo "========================================"
echo ""

# 创建文档目录结构
mkdir -p docs/consolidated/{architecture,performance,testing,docker,production,optimization,benchmark,guides,reports}

echo -e "${BLUE}创建文档目录结构完成${NC}"
echo ""

# 文档分类映射
# 架构文档
ARCHITECTURE_DOCS=(
    "ARCHITECTURE.md"
    "CORE_DESIGN_PRINCIPLES.md"
    "MICROSERVICES_ARCHITECTURE.md"
    "EVENT_SOURCING_CORE_DESIGN.md"
    "EVENT_SOURCING_AND_DETERMINISTIC_CALCULATION.md"
    "TRADING_SERVICE_CQRS_DESIGN.md"
    "TRADING_SERVICE_CQRS_ANALYSIS.md"
)

# 性能文档
PERFORMANCE_DOCS=(
    "PERFORMANCE_ANALYSIS_REPORT.md"
    "PERFORMANCE_COMPARISON.md"
    "PERFORMANCE_OPTIMIZATION_V2.md"
    "PERFORMANCE_TEST_REPORT.md"
    "PERFORMANCE_BENCHMARK_FINAL.md"
    "PERFORMANCE_BENCHMARK_REPORT.md"
    "PERFORMANCE_BENCHMARK_REPORT_20251209_223157.md"
    "PERFORMANCE_BENCHMARK_REPORT_20251209_224931.md"
    "PERFORMANCE_BENCHMARK_REPORT_20251209_225128.md"
    "PERFORMANCE_BENCHMARK_REPORT_20251209_225409.md"
    "PERFORMANCE_OPTIMIZATION_CONSISTENCY_REPORT.md"
    "PHASE1_PERFORMANCE_ANALYSIS.md"
    "PRODUCTION_PERFORMANCE_ANALYSIS.md"
    "EVENT_SOURCING_PERFORMANCE_REPORT.md"
    "EVENT_SOURCING_DOCKER_PERFORMANCE_REPORT.md"
    "ART_PERFORMANCE_COMPARISON.md"
    "ART_SIMD_OPTIMIZATION.md"
    "ART_SIMD_ENHANCED_OPTIMIZATIONS.md"
    "PERSISTENCE_OPTIMIZATION.md"
    "WAL_PERFORMANCE.txt"
    "WAL_VS_BATCH_ANALYSIS.md"
    "SAFETY_VS_PERFORMANCE.md"
)

# 测试文档
TESTING_DOCS=(
    "tests/TESTING_GUIDE.md"
    "tests/TEST_EXECUTION_REPORT.md"
    "tests/UNIT_TESTS_SUMMARY.md"
    "tests/functional/README.md"
    "tests/functional/CONSENSUS_TEST_README.md"
    "tests/unit/README.md"
)

# Docker文档
DOCKER_DOCS=(
    "DOCKER_QUICK_START.md"
    "DOCKER_BENCHMARK_README.md"
    "DOCKER_README.txt"
    "QUICK_START_DOCKER.md"
    "README_DOCKER.md"
)

# 生产环境文档
PRODUCTION_DOCS=(
    "PRODUCTION_READY.md"
    "PRODUCTION_FEATURES.md"
    "PRODUCTION_FEATURES_GAP_ANALYSIS.md"
    "PRODUCTION_FEATURES_IMPLEMENTATION.md"
    "PRODUCTION_V2_SUCCESS.md"
    "SECURITY.md"
    "WAL_GROUP_COMMIT_SAFETY.md"
    "WAL_CODE_ANALYSIS.md"
    "ASYNC_PERSISTENCE_SAFETY.md"
)

# 优化文档
OPTIMIZATION_DOCS=(
    "OPTIMIZATION_REPORT.md"
    "OPTIMIZATION_ROADMAP.md"
    "QUICK_OPTIMIZATION_REFERENCE.md"
    "OPTIMIZATION_JOURNEY.txt"
)

# 基准测试文档
BENCHMARK_DOCS=(
    "BENCHMARK_GUIDE.md"
    "BENCHMARK_GUIDE.zh-CN.md"
    "BENCHMARK_REPORT.md"
    "BENCHMARK_FINAL_REPORT.md"
    "FINAL_PERFORMANCE_REPORT.md"
    "FINAL_COMPARISON_REPORT.md"
    "FINAL_V3_COMPARISON.md"
    "COMPREHENSIVE_COMPARISON_REPORT.md"
    "VERSION_COMPARISON.md"
)

# 指南文档
GUIDES_DOCS=(
    "CONTRIBUTING.md"
    "GENERATE_REPORTS.md"
    "GITHUB_SETUP.md"
    "CHANGELOG.md"
)

# 报告文档
REPORTS_DOCS=(
    "FINAL_REPORT.md"
    "COMPILATION_SUCCESS_REPORT.md"
    "CODE_DOC_CONSISTENCY_REPORT.md"
    "GIT_HISTORY_CLEANUP_REPORT.md"
    "WIKI_CODE_CONSISTENCY_REPORT.md"
    "CLEANUP_REPORT.md"
    "ALL_REPORTS_INDEX.md"
)

echo -e "${BLUE}开始分类和合并文档...${NC}"
echo ""

# 函数：合并文档
merge_docs() {
    local category="$1"
    local output_file="$2"
    shift 2
    local docs=("$@")
    
    echo -e "${YELLOW}合并 $category 文档到 $output_file${NC}"
    
    # 创建合并文档
    {
        echo "# $category"
        echo ""
        echo "本文档合并了以下相关文档："
        echo ""
        for doc in "${docs[@]}"; do
            if [ -f "$doc" ]; then
                echo "- \`$doc\`"
            fi
        done
        echo ""
        echo "---"
        echo ""
        
        # 合并每个文档
        for doc in "${docs[@]}"; do
            if [ -f "$doc" ] && [ -s "$doc" ]; then
                echo ""
                echo "## 来源: $doc"
                echo ""
                cat "$doc"
                echo ""
                echo "---"
                echo ""
            fi
        done
    } > "$output_file"
    
    if [ -f "$output_file" ] && [ -s "$output_file" ]; then
        echo -e "${GREEN}✅ $output_file 创建成功${NC}"
    fi
}

# 合并各类文档
merge_docs "架构设计文档" "docs/consolidated/architecture/ARCHITECTURE_CONSOLIDATED.md" "${ARCHITECTURE_DOCS[@]}"
merge_docs "性能优化文档" "docs/consolidated/performance/PERFORMANCE_CONSOLIDATED.md" "${PERFORMANCE_DOCS[@]}"
merge_docs "测试文档" "docs/consolidated/testing/TESTING_CONSOLIDATED.md" "${TESTING_DOCS[@]}"
merge_docs "Docker文档" "docs/consolidated/docker/DOCKER_CONSOLIDATED.md" "${DOCKER_DOCS[@]}"
merge_docs "生产环境文档" "docs/consolidated/production/PRODUCTION_CONSOLIDATED.md" "${PRODUCTION_DOCS[@]}"
merge_docs "优化文档" "docs/consolidated/optimization/OPTIMIZATION_CONSOLIDATED.md" "${OPTIMIZATION_DOCS[@]}"
merge_docs "基准测试文档" "docs/consolidated/benchmark/BENCHMARK_CONSOLIDATED.md" "${BENCHMARK_DOCS[@]}"
merge_docs "指南文档" "docs/consolidated/guides/GUIDES_CONSOLIDATED.md" "${GUIDES_DOCS[@]}"
merge_docs "报告文档" "docs/consolidated/reports/REPORTS_CONSOLIDATED.md" "${REPORTS_DOCS[@]}"

echo ""
echo "========================================"
echo "文档合并完成"
echo "========================================"
echo ""
echo "合并后的文档位于: docs/consolidated/"
echo ""

