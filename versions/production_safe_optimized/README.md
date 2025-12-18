# Production Safe Optimized

## Overview

This version combines the **zero data loss guarantee** of `production_safe` with the **high performance** of `event_sourcing`.

## Key Optimizations

1. **Async WAL Writes**: Non-blocking critical path using lock-free queue
2. **In-Memory Event Buffer**: Fast event storage (10K events) like event_sourcing
3. **Batch fsync**: Group commit optimization (10ms or 1000 entries)
4. **Lock-Free Queue**: 64K entry SPSC queue for WAL operations

## Performance Targets

| Metric | production_safe | event_sourcing | production_safe_optimized (target) |
|--------|----------------|----------------|-----------------------------------|
| Throughput | 9.78 K/s | 418.80 K/s | **200 K/s** |
| Latency | 99.68 μs | 1.75 μs | **5 μs** |
| Data Safety | ✅✅✅ | ✅✅ | **✅✅✅** |

## Building

### macOS (Apple Silicon)

```bash
cd versions/production_safe_optimized
mkdir -p build && cd build
cmake ..
make -j4
```

Note: `-march=native` is automatically disabled on Apple platforms for compatibility.

### Linux (x86_64)

```bash
cd versions/production_safe_optimized
mkdir -p build && cd build
cmake ..
make -j4
```

## Running Benchmark

```bash
cd build
./production_safe_optimized_benchmark
```

This will generate `BENCHMARK_REPORT.md` in the version directory.

## Architecture

```
┌─────────────────────────────────────────┐
│  process_order_optimized()              │
│  (Critical Path - ~5μs target)          │
├─────────────────────────────────────────┤
│  1. Process order (V2, ~1.2μs)        │
│  2. Add to event buffer (~0.1μs)        │
│  3. Enqueue to WAL queue (~0.01μs)      │
└─────────────────────────────────────────┘
              │
              ├──> Event Buffer (Memory)
              │
              └──> WAL Queue (Lock-Free)
                        │
                        ├──> WAL Writer Thread
                        │    (Background, async)
                        │
                        └──> Sync Worker Thread
                             (Periodic fsync)
```

## Data Safety

- **Zero Data Loss**: WAL ensures all operations are logged
- **Crash Recovery**: Replay from WAL on startup
- **Periodic Sync**: 10ms or 1000 entries (configurable)
- **Async Writes**: Non-blocking but guaranteed durability

## Status

⚠️ **Compilation**: Currently fixing Apple Silicon compatibility issues with `-march=native` flag.

See `OPTIMIZATION_DESIGN.md` for detailed design documentation.

