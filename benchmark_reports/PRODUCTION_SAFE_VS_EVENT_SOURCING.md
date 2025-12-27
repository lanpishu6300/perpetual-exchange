# Production Safe vs Event Sourcing Performance Comparison

## Overview

This report compares the performance characteristics of two versions that prioritize data safety and auditability:

- **production_safe**: WAL-based zero data loss version
- **event_sourcing**: Event sourcing pattern implementation

[中文](PRODUCTION_SAFE_VS_EVENT_SOURCING.zh-CN.md) | [English](PRODUCTION_SAFE_VS_EVENT_SOURCING.md)

## Test Configuration

- **Test Orders**: 50,000 per version
- **Test Environment**: Docker (Linux amd64)
- **Compiler**: GCC with `-O3 -flto -funroll-loops`
- **Test Date**: 2025-12-18

---

## Performance Metrics Comparison

| Metric | production_safe | event_sourcing | Difference |
|--------|---------------|----------------|------------|
| **Throughput** | 9.78 K orders/sec | 418.80 K orders/sec | **event_sourcing is 42.8x faster** |
| **Average Latency** | 99.68 μs | 1.75 μs | **event_sourcing is 56.9x lower** |
| **P50 Latency** | 78.17 μs | 1.54 μs | **event_sourcing is 50.8x lower** |
| **P90 Latency** | 147.96 μs | 2.08 μs | **event_sourcing is 71.2x lower** |
| **P99 Latency** | 397.00 μs | 3.08 μs | **event_sourcing is 128.9x lower** |
| **Min Latency** | 30.42 μs | 0.96 μs | **event_sourcing is 31.7x lower** |
| **Max Latency** | 46,788.50 μs | 2,344.33 μs | **event_sourcing is 20.0x lower** |
| **Total Trades** | 12 | 10 | Similar trade execution |
| **Trade Rate** | 0.02% | 0.02% | Identical |
| **Errors** | 0 | 0 | Both error-free |

---

## Detailed Analysis

### Throughput Analysis

**production_safe**: 9.78 K orders/sec
- Lower throughput due to WAL (Write-Ahead Log) overhead
- Every operation must be logged to disk before completion
- Group commit optimization helps but still has significant overhead

**event_sourcing**: 418.80 K orders/sec
- Much higher throughput with in-memory event storage
- Events are stored asynchronously without blocking matching
- Optimized for high-frequency trading scenarios

**Performance Gap**: event_sourcing processes **42.8x more orders per second**

### Latency Analysis

**production_safe**:
- Average: 99.68 μs (nearly 100 microseconds)
- P99: 397.00 μs (nearly 400 microseconds)
- High latency due to synchronous WAL writes
- Disk I/O operations add significant overhead

**event_sourcing**:
- Average: 1.75 μs (sub-2 microsecond latency)
- P99: 3.08 μs (sub-4 microsecond latency)
- Ultra-low latency with in-memory operations
- No disk I/O blocking the critical path

**Performance Gap**: event_sourcing has **56.9x lower average latency**

### Latency Distribution

#### production_safe
- **P50**: 78.17 μs - Half of orders complete in ~78μs
- **P90**: 147.96 μs - 90% of orders complete in ~148μs
- **P99**: 397.00 μs - 99% of orders complete in ~397μs
- **Max**: 46,788.50 μs - Occasional spikes up to ~47ms

#### event_sourcing
- **P50**: 1.54 μs - Half of orders complete in ~1.5μs
- **P90**: 2.08 μs - 90% of orders complete in ~2μs
- **P99**: 3.08 μs - 99% of orders complete in ~3μs
- **Max**: 2,344.33 μs - Maximum latency ~2.3ms

**Key Insight**: event_sourcing shows much tighter latency distribution with minimal tail latency.

---

## Feature Comparison

| Feature | production_safe | event_sourcing |
|---------|---------------|----------------|
| **Data Safety** | ✅✅✅ Zero data loss (WAL) | ✅✅ Event replay capability |
| **Crash Recovery** | ✅✅✅ Automatic from WAL | ✅✅ Replay events from log |
| **Audit Trail** | ✅✅ WAL log | ✅✅✅ Complete event history |
| **Performance** | ⚠️ Lower (9.78K/s) | ✅✅✅ High (418.80K/s) |
| **Latency** | ⚠️ Higher (99.68μs) | ✅✅✅ Ultra-low (1.75μs) |
| **Deterministic** | ✅✅✅ Yes | ✅✅✅ Yes (event replay) |
| **Use Case** | Financial systems requiring zero data loss | High-frequency trading, audit requirements |

---

## Use Case Recommendations

### Choose production_safe when:
- **Zero data loss is critical** (regulatory requirements)
- **Crash recovery must be guaranteed** (financial systems)
- **Throughput requirements are moderate** (< 20K orders/sec)
- **Latency requirements are relaxed** (> 50μs acceptable)
- **Compliance and audit trails** are mandatory

### Choose event_sourcing when:
- **Ultra-low latency is critical** (< 5μs)
- **High throughput is required** (> 300K orders/sec)
- **Complete audit trail** is needed (event history)
- **Deterministic replay** is important
- **In-memory performance** is acceptable
- **Event-based architecture** fits the system design

---

## Performance Trade-offs

### production_safe Trade-offs
- ✅ **Pros**: Zero data loss, guaranteed persistence, crash recovery
- ❌ **Cons**: 42.8x slower throughput, 56.9x higher latency, disk I/O overhead

### event_sourcing Trade-offs
- ✅ **Pros**: 42.8x faster throughput, 56.9x lower latency, complete audit trail
- ❌ **Cons**: Requires separate persistence strategy, memory usage for event log

---

## Technical Implementation Differences

### production_safe
- **WAL Mechanism**: Write-Ahead Logging with group commit
- **Persistence**: Synchronous disk writes before operation completion
- **Recovery**: Replay WAL entries on startup
- **Overhead**: Disk I/O on critical path

### event_sourcing
- **Event Storage**: In-memory event log with async persistence
- **Persistence**: Asynchronous event logging (non-blocking)
- **Recovery**: Replay events from event store
- **Overhead**: Minimal (in-memory operations)

---

## Conclusion

**For High-Performance Trading**:
- **event_sourcing** is the clear winner with 418.80K orders/sec and 1.75μs latency
- Suitable for high-frequency trading where speed is critical
- Provides complete audit trail through event history

**For Financial Compliance**:
- **production_safe** provides zero data loss guarantee
- Essential for systems where data loss is unacceptable
- Acceptable performance for moderate throughput requirements

**Hybrid Approach**:
Consider using **event_sourcing** for matching engine performance with **production_safe**-style WAL for critical state changes, combining the best of both approaches.

---

## Related Reports

- [production_safe Report](./docker/production_safe_BENCHMARK_REPORT.md)
- [event_sourcing Report](./docker/event_sourcing_BENCHMARK_REPORT.md)
- [Cross-Platform Report](./CROSS_PLATFORM_BENCHMARK_REPORT.md)

---

*Report generated from Docker benchmark results*



