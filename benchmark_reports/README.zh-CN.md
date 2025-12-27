# 压测报告

本目录包含所有版本的性能压测报告。

[English](README.md) | [中文](README.zh-CN.md)

## 目录结构

```
benchmark_reports/
├── mac/                    # Mac平台报告
│   ├── original_BENCHMARK_REPORT.md
│   ├── optimized_BENCHMARK_REPORT.md
│   ├── optimized_v2_BENCHMARK_REPORT.md
│   ├── art_BENCHMARK_REPORT.md
│   └── production_basic_BENCHMARK_REPORT.md
├── docker/                 # Docker平台报告
│   ├── original_BENCHMARK_REPORT.md
│   ├── optimized_BENCHMARK_REPORT.md
│   ├── optimized_v2_BENCHMARK_REPORT.md
│   ├── art_BENCHMARK_REPORT.md
│   ├── art_simd_BENCHMARK_REPORT.md
│   ├── event_sourcing_BENCHMARK_REPORT.md
│   ├── production_basic_BENCHMARK_REPORT.md
│   ├── production_fast_BENCHMARK_REPORT.md
│   └── production_safe_BENCHMARK_REPORT.md
├── CROSS_PLATFORM_BENCHMARK_REPORT.md  # 跨平台对比报告
└── BENCHMARK_COMPARISON_ANALYSIS.md    # 性能分析报告
```

## 报告类型

### 平台报告

- **Mac报告** (`mac/`): 在macOS环境下运行的压测结果
- **Docker报告** (`docker/`): 在Docker容器（Linux）环境下运行的压测结果

### 对比报告

- **跨平台报告**: Mac和Docker两个平台的性能对比
- **性能对比分析**: 所有版本的综合性能分析

## 测试配置

- **测试订单数**: 每个版本50,000单
- **编译优化**: `-O3 -flto -funroll-loops`
- **测试环境**: 
  - Mac: macOS 14.4.1, Apple M1 Pro, Clang/LLVM
  - Docker: Linux amd64, GCC

## 版本说明

项目包含9个不同版本的撮合引擎实现：

1. **original** - 基础实现
2. **optimized** - 内存池优化版本
3. **optimized_v2** - 热路径优化版本
4. **art** - 自适应基数树实现
5. **art_simd** - ART + SIMD优化版本
6. **event_sourcing** - 事件溯源实现
7. **production_basic** - 完整生产版本
8. **production_fast** - 高性能生产版本
9. **production_safe** - WAL安全版本（零数据丢失）

## 性能指标

每个报告包含以下性能指标：

- **吞吐量** (Throughput): 每秒处理的订单数（K orders/sec）
- **延迟** (Latency): 平均处理延迟（微秒）
- **百分位延迟**: P50, P90, P99
- **交易统计**: 总交易数、交易率等

## 查看报告

所有报告均为Markdown格式，可直接在GitHub或任何Markdown查看器中查看。

最新的跨平台对比报告请参见 [CROSS_PLATFORM_BENCHMARK_REPORT.md](CROSS_PLATFORM_BENCHMARK_REPORT.md)。



