# Original 性能 Benchmark Report

## 测试概述

- **Version**: Original
- **测试日期**: 1766042550
- **Total Orders**: 49000
- **Duration**: 163 ms

## 结果

| Metric | Value |
|--------|-------|
| 吞吐量 | 300.61 K orders/sec |
| Total Trades | 9 |
| Trade Rate | 0.02 % |
| Errors | 0 |

## 延迟 Statistics

| Percentile | 延迟 (μs) |
|------------|---------------|
| 平均 | 2.49 |
| 最小 | 0.92 |
| 最大 | 4004.83 |
| P50 | 2.00 |
| P90 | 2.67 |
| P99 | 5.96 |

## Version Characteristics

- **Implementation**: Red-Black Tree
- **性能 Target**: ~300K orders/sec, ~3μs latency
- **Use Case**: Baseline benchmark

