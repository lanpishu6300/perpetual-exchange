#!/bin/bash

cd build

echo "=========================================="
echo "运行快速性能测试 (5秒)"
echo "=========================================="

# 运行快速测试，限制时间
if [ -f test_optimized_v3_quick ]; then
    ./test_optimized_v3_quick 2>&1 | tee /tmp/quick_perf_test.txt
elif [ -f test_optimized_v3_minimal ]; then
    ./test_optimized_v3_minimal 2>&1 | tee /tmp/quick_perf_test.txt
else
    echo "快速测试可执行文件不存在"
    exit 1
fi

echo ""
echo "测试完成，输出已保存到 /tmp/quick_perf_test.txt"
