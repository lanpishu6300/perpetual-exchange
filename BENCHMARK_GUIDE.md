# Benchmark Guide

Performance testing guide for all versions of the matching engine.

[中文](BENCHMARK_GUIDE.zh-CN.md) | [English](BENCHMARK_GUIDE.md)

## Quick Start

### Mac Platform

```bash
./run_mac_benchmarks.sh 50000
```

Reports will be generated in `benchmark_reports/mac/` directory.

### Docker Platform

```bash
docker compose -f docker-compose.benchmark.yml run --rm benchmark-runner /app/run_all_benchmarks.sh 50000
```

Reports will be generated in `benchmark_reports/docker/` directory.

## Viewing Reports

All reports are located in `benchmark_reports/` directory:

- `mac/` - Mac platform reports
- `docker/` - Docker platform reports  
- `CROSS_PLATFORM_BENCHMARK_REPORT.md` - Cross-platform comparison report

## Report Format

Each report includes:

- Test overview (version, date, order count)
- Performance metrics (throughput, latency)
- Latency statistics (average, min, max, P50/P90/P99)
- Version-specific characteristics

## Customization

You can customize the number of test orders:

```bash
# Test with 100,000 orders
./run_mac_benchmarks.sh 100000

# Docker with 100,000 orders
docker compose -f docker-compose.benchmark.yml run --rm benchmark-runner /app/run_all_benchmarks.sh 100000
```

## Troubleshooting

### Build Issues

If benchmarks fail to build, ensure:
- C++17 compiler is installed
- CMake 3.15+ is available
- All dependencies are installed

### Docker Issues

If Docker benchmarks fail:
- Ensure Docker is running
- Check Docker Compose version
- Verify platform compatibility (linux/amd64)



