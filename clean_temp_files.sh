#!/bin/bash

# 清理中间文件、日志和WAL脚本
# 安全地删除临时文件、日志文件和WAL文件

# 不使用 set -e，因为某些文件可能不存在是正常的

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo "========================================"
echo "清理中间文件、日志和WAL"
echo "========================================"
echo ""

# 统计变量
total_size=0
files_deleted=0
dirs_deleted=0

# 函数：计算目录大小
calculate_size() {
    if [ -d "$1" ]; then
        du -sk "$1" 2>/dev/null | awk '{print $1}'
    elif [ -f "$1" ]; then
        du -sk "$1" 2>/dev/null | awk '{print $1}'
    else
        echo "0"
    fi
}

# 函数：安全删除
safe_delete() {
    local target="$1"
    local desc="$2"
    
    if [ -e "$target" ]; then
        local size=$(calculate_size "$target" 2>/dev/null || echo "0")
        total_size=$((total_size + size))
        
        if [ -d "$target" ]; then
            echo -e "${YELLOW}删除目录:${NC} $target ($desc)"
            rm -rf "$target" 2>/dev/null && ((dirs_deleted++)) || true
        else
            echo -e "${YELLOW}删除文件:${NC} $target ($desc)"
            rm -f "$target" 2>/dev/null && ((files_deleted++)) || true
        fi
    fi
}

echo -e "${BLUE}1. 清理日志文件 (.log)${NC}"
echo "----------------------------------------"
# 根目录下的日志文件
for log_file in *.log; do
    if [ -f "$log_file" ]; then
        safe_delete "$log_file" "日志文件"
    fi
done

# versions目录下的日志文件
find versions -type f -name "*.log" 2>/dev/null | while read -r log_file; do
    safe_delete "$log_file" "版本日志文件"
done

echo ""

echo -e "${BLUE}2. 清理WAL文件${NC}"
echo "----------------------------------------"
# 查找并删除WAL目录
find . -type d -name "wal" 2>/dev/null | while read -r wal_dir; do
    # 排除.git目录
    if [[ ! "$wal_dir" =~ \.git ]]; then
        safe_delete "$wal_dir" "WAL目录"
    fi
done

# 查找并删除WAL日志文件
find . -type f -name "wal.log" -o -name "*.wal" 2>/dev/null | while read -r wal_file; do
    if [[ ! "$wal_file" =~ \.git ]]; then
        safe_delete "$wal_file" "WAL日志文件"
    fi
done

echo ""

echo -e "${BLUE}3. 清理临时测试目录${NC}"
echo "----------------------------------------"
# test_v3_* 目录
for test_dir in test_v3_* test_orderflow_* test_functional_event_*; do
    if [ -d "$test_dir" ]; then
        safe_delete "$test_dir" "临时测试目录"
    fi
done

echo ""

echo -e "${BLUE}4. 清理测试结果目录${NC}"
echo "----------------------------------------"
safe_delete "test_results" "测试结果目录"

# versions目录下的测试结果
find versions -type d -name "test_results" -o -name "*_test_results" 2>/dev/null | while read -r test_result_dir; do
    safe_delete "$test_result_dir" "版本测试结果目录"
done

echo ""

echo -e "${BLUE}5. 清理数据文件（orders.log, trades.log等）${NC}"
echo "----------------------------------------"
# data目录下的日志文件
find data versions/*/data -type f \( -name "orders.log" -o -name "trades.log" -o -name "*.log" \) 2>/dev/null | while read -r data_log; do
    safe_delete "$data_log" "数据日志文件"
done

echo ""

echo -e "${BLUE}6. 清理编译中间文件${NC}"
echo "----------------------------------------"
# .o文件
find . -type f -name "*.o" ! -path "./.git/*" ! -path "./build/*" 2>/dev/null | while read -r obj_file; do
    safe_delete "$obj_file" "目标文件"
done

# .a文件（静态库，但保留build目录中的）
find . -type f -name "*.a" ! -path "./.git/*" ! -path "./build/*" 2>/dev/null | while read -r lib_file; do
    safe_delete "$lib_file" "静态库文件"
done

echo ""

echo -e "${BLUE}7. 清理备份文件${NC}"
echo "----------------------------------------"
find . -type f \( -name "*.bak" -o -name "*.bak*" -o -name "*~" \) ! -path "./.git/*" 2>/dev/null | while read -r bak_file; do
    safe_delete "$bak_file" "备份文件"
done

echo ""

echo -e "${BLUE}8. 清理临时文件${NC}"
echo "----------------------------------------"
find . -type f \( -name "*.tmp" -o -name "*.swp" -o -name "*.swo" \) ! -path "./.git/*" 2>/dev/null | while read -r tmp_file; do
    safe_delete "$tmp_file" "临时文件"
done

echo ""

echo -e "${BLUE}9. 清理CMake缓存文件（保留build目录）${NC}"
echo "----------------------------------------"
# 只清理根目录下的CMake缓存，不清理build目录
for cmake_file in CMakeCache.txt cmake_install.cmake; do
    if [ -f "$cmake_file" ]; then
        safe_delete "$cmake_file" "CMake缓存文件"
    fi
done

# 清理CMakeFiles目录（除了build目录中的）
find . -type d -name "CMakeFiles" ! -path "./.git/*" ! -path "./build/*" 2>/dev/null | while read -r cmake_dir; do
    safe_delete "$cmake_dir" "CMakeFiles目录"
done

echo ""

# 显示统计信息
echo "========================================"
echo "清理完成"
echo "========================================"
echo -e "${GREEN}删除文件数:${NC} $files_deleted"
echo -e "${GREEN}删除目录数:${NC} $dirs_deleted"
if [ $total_size -gt 0 ]; then
    size_mb=$((total_size / 1024))
    echo -e "${GREEN}释放空间:${NC} ${size_mb} MB"
fi
echo ""

echo -e "${GREEN}✅ 清理完成！${NC}"

