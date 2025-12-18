# Cross-Platform Build Guide

## Overview

This project supports building and running benchmarks on both **Mac** and **Docker (Linux)** environments.

## Quick Start

### Mac

```bash
# Build all versions
./build_all_versions.sh

# Run a specific benchmark
cd versions/original
./build/original_benchmark 50000
```

### Docker

```bash
# Build and run all benchmarks
docker compose -f docker-compose.benchmark.yml run --rm benchmark-runner /app/run_all_benchmarks.sh 50000
```

## Platform Differences

### Mac (macOS)
- Uses `sysctl -n hw.ncpu` for CPU count
- Native build with Clang/LLVM
- No SIMD restrictions (uses native instructions)

### Docker (Linux)
- Uses `nproc` for CPU count
- GCC compiler
- SIMD support via `-mavx2` flag

## Fixed Issues

1. **Include Paths**: All versions now use `core/` prefix for includes
2. **Header Dependencies**: Required headers copied to each version's local `include/core/`
3. **Source Files**: Missing source files copied to version directories
4. **Atomic Headers**: Added `<atomic>` where needed
5. **SIMD Compatibility**: Conditional SIMD flags based on platform

## Successfully Running Versions

✅ **4 versions** successfully run in Docker:
- original
- optimized
- optimized_v2
- art

## Build Scripts

- `build_all_versions.sh` - Cross-platform build script
- `run_all_benchmarks.sh` - Docker benchmark runner
- `Dockerfile.benchmark` - Docker build configuration

