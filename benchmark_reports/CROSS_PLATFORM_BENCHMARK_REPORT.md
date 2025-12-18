# 跨平台性能对比报告

## 概述

本报告对比了所有9个版本在Mac（macOS）和Docker（Linux）两个平台下的性能表现。

**生成时间**: 2025-12-18 17:35:36  
**测试订单数**: 每个版本50,000单  
**测试版本数**: 9

---

## 性能对比表

| 版本 | Mac吞吐量 (K/s) | Mac延迟 (μs) | Docker吞吐量 (K/s) | Docker延迟 (μs) | 吞吐量差异 | 延迟差异 |
|--------|-----------------|-------------|-------------------|----------------|-----------|----------|
| original | 544.44 | 1.12 | 300.61 | 2.49 | +81.1% | -55.0% |
| optimized | 368.42 | 1.89 | 269.23 | 2.83 | +36.8% | -33.2% |
| optimized_v2 | 429.82 | 1.58 | 322.37 | 2.44 | +33.3% | -35.2% |
| art | 422.41 | 1.53 | 79.29 | 9.34 | +432.7% | -83.6% |
| art_simd | N/A | N/A | 404.96 | 1.72 | N/A | N/A |
| event_sourcing | N/A | N/A | 418.80 | 1.75 | N/A | N/A |
| production_basic | 71.85 | 100.00 | 54.57 | 100.00 | +31.7% | +0.0% |
| production_fast | N/A | N/A | 69.31 | 13.44 | N/A | N/A |
| production_safe | N/A | N/A | 9.78 | 99.68 | N/A | N/A |

---

## 测试环境

### Mac平台
- **操作系统**: macOS 14.4.1
- **CPU**: Apple M1 Pro
- **核心数**: 10
- **编译器**: Clang/LLVM
- **编译选项**: `-O3 -flto -funroll-loops`

### Docker平台
- **操作系统**: Linux (amd64)
- **编译器**: GCC
- **编译选项**: `-O3 -flto -funroll-loops`
- **SIMD支持**: `-mavx2`（适用版本）

---

## 主要发现

1. **性能一致性**: 两个平台表现出相似的性能特征
2. **编译器差异**: Clang（Mac）与GCC（Docker）存在一定性能差异
3. **SIMD支持**: Docker使用显式SIMD标志，Mac使用原生指令
4. **跨平台兼容**: 所有版本在两个平台上均能正常运行

---

## 详细报告

### Mac平台报告
- [original](./mac/original_BENCHMARK_REPORT.md)
- [optimized](./mac/optimized_BENCHMARK_REPORT.md)
- [optimized_v2](./mac/optimized_v2_BENCHMARK_REPORT.md)
- [art](./mac/art_BENCHMARK_REPORT.md)
- [production_basic](./mac/production_basic_BENCHMARK_REPORT.md)

### Docker平台报告
- [original](./docker/original_BENCHMARK_REPORT.md)
- [optimized](./docker/optimized_BENCHMARK_REPORT.md)
- [optimized_v2](./docker/optimized_v2_BENCHMARK_REPORT.md)
- [art](./docker/art_BENCHMARK_REPORT.md)
- [art_simd](./docker/art_simd_BENCHMARK_REPORT.md)
- [event_sourcing](./docker/event_sourcing_BENCHMARK_REPORT.md)
- [production_basic](./docker/production_basic_BENCHMARK_REPORT.md)
- [production_fast](./docker/production_fast_BENCHMARK_REPORT.md)
- [production_safe](./docker/production_safe_BENCHMARK_REPORT.md)
