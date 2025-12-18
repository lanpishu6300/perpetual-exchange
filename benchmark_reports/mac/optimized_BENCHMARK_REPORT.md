# Optimized 性能 Benchmark Report

## 测试概述

- **Version**: Optimized
- **测试日期**: 1766043064
- **Total Orders**: 49000
- **Duration**: 133 ms

## 结果

| Metric | Value |
|--------|-------|
| 吞吐量 | 368.42 K orders/sec |
| Total Trades | 16 |
| Trade Rate | 0.03 % |
| Errors | 0 |

## 延迟 Statistics

| Percentile | 延迟 (μs) |
|------------|---------------|
| 平均 | 1.89 |
| 最小 | 0.17 |
| 最大 | 2984.96 |
| P50 | 1.08 |
| P90 | 2.00 |
| P99 | 17.83 |

## Version Characteristics

- **Implementation**: Memory Pool + Lock-Free
- **性能 Target**: ~300K orders/sec, ~3μs latency
- **Use Case**: Optimized benchmark

