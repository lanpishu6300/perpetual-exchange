# 生成对比报告指南

## 🎯 快速生成报告

### 方法1: 运行完整对比程序

```bash
cd build
./comprehensive_performance_comparison
```

这会生成：
- `comprehensive_performance_report.txt` - 详细数值报告

### 方法2: 运行所有基准测试

```bash
./run_all_benchmarks.sh
```

这会生成：
- `reports/original_benchmark.txt`
- `reports/optimized_comparison.txt`
- `reports/persistence_benchmark.txt`
- `reports/comprehensive_comparison.txt`

## 📊 报告内容

### 性能对比报告包含

1. **性能摘要表**
   - 吞吐量对比
   - 延迟对比（平均、P99）
   - 成交率对比

2. **延迟分布**
   - Min, P50, P90, P99, Max
   - 各版本的完整延迟分布

3. **改进分析**
   - 相对于基准的改进百分比
   - 吞吐量提升
   - 延迟降低

## 📝 查看报告

### 查看文本报告

```bash
cat comprehensive_performance_report.txt
```

### 查看Markdown报告

```bash
cat COMPREHENSIVE_COMPARISON_REPORT.md
cat FINAL_PERFORMANCE_REPORT.md
cat VERSION_COMPARISON.md
```

## 🔧 自定义测试

### 修改测试规模

编辑 `src/comprehensive_performance_comparison.cpp`:

```cpp
const size_t num_orders = 50000; // 修改为你需要的数量
```

### 添加新版本测试

在 `main()` 函数中添加：

```cpp
auto result_new = comparator.benchmark<YourEngineType>(
    "Your Version Name", num_orders, instrument_id);
comparator.addResult(result_new);
```

## ✅ 报告文件清单

### 自动生成的报告
- `comprehensive_performance_report.txt` - 数值报告

### 文档报告
- `COMPREHENSIVE_COMPARISON_REPORT.md` - 完整对比
- `FINAL_PERFORMANCE_REPORT.md` - 最终报告
- `VERSION_COMPARISON.md` - 版本对比
- `ALL_VERSIONS_SUMMARY.md` - 版本总结
- `README_VERSIONS.md` - 使用指南

---

**状态**: ✅ 报告生成工具已就绪
**最后更新**: 2024年12月



