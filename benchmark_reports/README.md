# Benchmark Reports

This directory contains performance benchmark reports for all versions.

[中文](README.zh-CN.md) | [English](README.md)

## Directory Structure

```
benchmark_reports/
├── mac/                    # Mac platform reports
│   ├── original_BENCHMARK_REPORT.md
│   ├── optimized_BENCHMARK_REPORT.md
│   ├── optimized_v2_BENCHMARK_REPORT.md
│   ├── art_BENCHMARK_REPORT.md
│   └── production_basic_BENCHMARK_REPORT.md
├── docker/                 # Docker platform reports
│   ├── original_BENCHMARK_REPORT.md
│   ├── optimized_BENCHMARK_REPORT.md
│   ├── optimized_v2_BENCHMARK_REPORT.md
│   ├── art_BENCHMARK_REPORT.md
│   ├── art_simd_BENCHMARK_REPORT.md
│   ├── event_sourcing_BENCHMARK_REPORT.md
│   ├── production_basic_BENCHMARK_REPORT.md
│   ├── production_fast_BENCHMARK_REPORT.md
│   └── production_safe_BENCHMARK_REPORT.md
├── CROSS_PLATFORM_BENCHMARK_REPORT.md  # Cross-platform comparison
└── BENCHMARK_COMPARISON_ANALYSIS.md    # Performance analysis
```

## Report Types

### Platform Reports

- **Mac Reports** (`mac/`): Benchmark results run on macOS
- **Docker Reports** (`docker/`): Benchmark results run in Docker containers (Linux)

### Comparison Reports

- **Cross-Platform Report**: Performance comparison between Mac and Docker platforms
- **Benchmark Comparison Analysis**: Comprehensive performance analysis across all versions

## Test Configuration

- **Test Orders**: 50,000 per version
- **Compilation Flags**: `-O3 -flto -funroll-loops`
- **Test Environments**: 
  - Mac: macOS 14.4.1, Apple M1 Pro, Clang/LLVM
  - Docker: Linux amd64, GCC

## Version Information

The project includes 9 different versions of the matching engine:

1. **original** - Baseline implementation
2. **optimized** - Memory pool optimized version
3. **optimized_v2** - Hot path optimized version
4. **art** - Adaptive Radix Tree implementation
5. **art_simd** - ART + SIMD optimized version
6. **event_sourcing** - Event sourcing implementation
7. **production_basic** - Full production version
8. **production_fast** - High-performance production version
9. **production_safe** - WAL-based zero data loss version

## Performance Metrics

Each report includes the following performance metrics:

- **Throughput**: Orders processed per second (K orders/sec)
- **Latency**: Average processing latency (microseconds)
- **Percentile Latency**: P50, P90, P99
- **Trade Statistics**: Total trades, trade rate, etc.

## Viewing Reports

All reports are in Markdown format and can be viewed directly in GitHub or any Markdown viewer.

For the latest cross-platform comparison, see [CROSS_PLATFORM_BENCHMARK_REPORT.md](CROSS_PLATFORM_BENCHMARK_REPORT.md).
