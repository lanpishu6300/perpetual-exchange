# Original 性能 Benchmark Report

## 测试概述

- **Version**: Original
- **测试日期**: 1766043057
- **Total Orders**: 49000
- **Duration**: 90 ms

## 结果

| Metric | Value |
|--------|-------|
| 吞吐量 | 544.44 K orders/sec |
| Total Trades | 16 |
| Trade Rate | 0.03 % |
| Errors | 0 |

## 延迟 Statistics

| Percentile | 延迟 (μs) |
|------------|---------------|
| 平均 | 1.12 |
| 最小 | 0.17 |
| 最大 | 1307.38 |
| P50 | 0.54 |
| P90 | 1.21 |
| P99 | 16.88 |

## Version Characteristics

- **Implementation**: Red-Black Tree
- **性能 Target**: ~300K orders/sec, ~3μs latency
- **Use Case**: Baseline benchmark

