# Docker Benchmark 运行状态

## ✅ 成功运行的版本

在Docker Linux环境中成功运行并生成真实benchmark报告的版本：

1. **original** - 350.00 K orders/sec, 2.18 μs 延迟
2. **art** - 347.52 K orders/sec, 2.13 μs 延迟

## 📊 真实Benchmark结果

### Original版本
- **吞吐量**: 350.00 K orders/sec
- **平均延迟**: 2.18 μs
- **P50延迟**: 1.79 μs
- **P90延迟**: 2.29 μs
- **P99延迟**: 5.17 μs
- **交易率**: 0.03%

### ART版本
- **吞吐量**: 347.52 K orders/sec
- **平均延迟**: 2.13 μs
- **P50延迟**: 1.54 μs
- **P90延迟**: 2.08 μs
- **P99延迟**: 5.58 μs
- **交易率**: 0.00%

## ⚠️ 需要修复的版本

以下版本在Docker中构建失败，需要修复依赖问题：

- optimized - 缺少 `core/thread_local_memory_pool.h`
- optimized_v2 - 依赖optimized版本
- art_simd - 链接错误
- event_sourcing - 缺少依赖文件
- production_basic - 链接错误
- production_fast - 链接错误
- production_safe - 链接错误

## 📁 报告位置

所有报告保存在 `benchmark_reports/` 目录：
- `original_BENCHMARK_REPORT.md` - ✅ 真实Docker运行结果
- `art_BENCHMARK_REPORT.md` - ✅ 真实Docker运行结果
- `BENCHMARK_COMPARISON_ANALYSIS.md` - 综合对比分析

## 🚀 运行方式

```bash
# 运行所有版本benchmark（实时输出）
./run_benchmarks_step_by_step.sh 50000

# 或使用docker compose
docker compose -f docker-compose.benchmark.yml run --rm benchmark-runner /app/run_all_benchmarks.sh 50000
```

## 📝 实时输出日志

运行过程的实时输出保存在：
- `docker_benchmark_realtime.log` - 完整运行日志
- `docker_all_benchmarks_realtime.log` - 所有版本运行日志

