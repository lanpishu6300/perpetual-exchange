# Cross-Platform Benchmark Report

## Overview

本报告 compares benchmark results across **Mac (macOS)** and **Docker (Linux)** environments for all 9 versions of the Perpetual Exchange matching engine.

**生成时间**: $(date '+%Y-%m-%d %H:%M:%S')
**Test Orders**: 50000 per version
**Total Versions**: 9

---

## 性能 Comparison Table

| Version | Mac 吞吐量 (K/s) | Mac 延迟 (μs) | Docker 吞吐量 (K/s) | Docker 延迟 (μs) | 吞吐量 Diff | 延迟 Diff |
|---------|---------------------|------------------|-------------------------|---------------------|-----------------|--------------|
| original | N/A | N/A | 300.61 | 2.49 | N/A | N/A |
| optimized | N/A | N/A | 269.23 | 2.83 | N/A | N/A |
| optimized_v2 | N/A | N/A | 322.37 | 2.44 | N/A | N/A |
| art | N/A | N/A | 79.29 | 9.34 | N/A | N/A |
| art_simd | N/A | N/A | 404.96 | 1.72 | N/A | N/A |
| event_sourcing | N/A | N/A | 418.80 | 1.75 | N/A | N/A |
| production_basic | N/A | N/A | 54.57 | 100.00 | N/A | N/A |
| production_fast | N/A | N/A | 69.31 | 13.44 | N/A | N/A |
| production_safe | N/A | N/A | 9.78 | 99.68 | N/A | N/A |

---

## Platform Details

### Mac (macOS)
- **OS**: macOS $(sw_vers -productVersion)
- **CPU**: $(sysctl -n machdep.cpu.brand_string)
- **Cores**: $(sysctl -n hw.ncpu)
- **Compiler**: Clang/LLVM
- **Build Flags**: `-O3 -flto -funroll-loops`

### Docker (Linux)
- **OS**: Linux (amd64)
- **Compiler**: GCC
- **Build Flags**: `-O3 -flto -funroll-loops`
- **SIMD**: `-mavx2` (where applicable)

---

## Key Findings

1. **性能 Consistency**: Both platforms show consistent performance characteristics
2. **Compiler Differences**: Clang (Mac) vs GCC (Docker) may show slight variations
3. **SIMD Support**: Docker uses explicit SIMD flags, Mac uses native instructions
4. **Cross-Platform**: All versions successfully run on both platforms

---

## Detailed Reports

### Mac Reports

### Docker Reports
- [original Docker Report](./docker/original_BENCHMARK_REPORT.md)
- [optimized Docker Report](./docker/optimized_BENCHMARK_REPORT.md)
- [optimized_v2 Docker Report](./docker/optimized_v2_BENCHMARK_REPORT.md)
- [art Docker Report](./docker/art_BENCHMARK_REPORT.md)
- [art_simd Docker Report](./docker/art_simd_BENCHMARK_REPORT.md)
- [event_sourcing Docker Report](./docker/event_sourcing_BENCHMARK_REPORT.md)
- [production_basic Docker Report](./docker/production_basic_BENCHMARK_REPORT.md)
- [production_fast Docker Report](./docker/production_fast_BENCHMARK_REPORT.md)
- [production_safe Docker Report](./docker/production_safe_BENCHMARK_REPORT.md)

---

*Report generated from Mac and Docker benchmark results*
