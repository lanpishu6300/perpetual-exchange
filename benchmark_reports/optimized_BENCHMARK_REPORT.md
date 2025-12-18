# Optimized 性能 Benchmark Report

## 测试概述

- **Version**: Optimized
- **测试日期**: 1766042551
- **Total Orders**: 49000
- **Duration**: 182 ms

## 结果

| Metric | Value |
|--------|-------|
| 吞吐量 | 269.23 K orders/sec |
| Total Trades | 22 |
| Trade Rate | 0.04 % |
| Errors | 0 |

## 延迟 Statistics

| Percentile | 延迟 (μs) |
|------------|---------------|
| 平均 | 2.83 |
| 最小 | 0.75 |
| 最大 | 4102.67 |
| P50 | 2.21 |
| P90 | 3.00 |
| P99 | 7.33 |

## Version Characteristics

- **Implementation**: Memory Pool + Lock-Free
- **性能 Target**: ~300K orders/sec, ~3μs latency
- **Use Case**: Optimized benchmark

