# Production Fast Benchmark Report

========================================
Production Fast Performance Benchmark Report
========================================
Version: production_fast
Test Date: 1766042555

Test Configuration:
  Total Orders:    49000
  Duration:        707 ms

Results:
  Throughput:      69.31 K orders/sec
  Total Trades:    16
  Trade Rate:      0.03 %
  Errors:          0

Latency Statistics:
  Average:       13.44 μs
  Minimum:       1.38 μs
  Maximum:       4631.67 μs
  P50:           16.67 μs
  P90:           18.42 μs
  P99:           53.29 μs

Performance Comparison:
  vs Original (300K/s):   ⚠️  76.90% slower
  vs ART+SIMD (750K/s):   ⚠️  90.76% slower
  vs Production Basic (15K/s):  ✅ 362.05% faster 🎉

========================================

## Engine Metrics

```
orders_received=50000
orders_processed=50000
orders_rejected=0
trades_executed=31
```
