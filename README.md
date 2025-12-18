# Perpetual Exchange - High-Performance Matching Engine

A production-ready perpetual futures exchange matching engine with nanosecond-level latency, featuring advanced optimizations including memory pooling, lock-free data structures, SIMD acceleration, and optimized persistence.

[中文文档](README.zh-CN.md) | [English](README.md)

## 🚀 Features

### Core Trading Features
- ✅ Order book management (Red-Black Tree, ART, O(log n))
- ✅ Price-time priority matching engine (nanosecond latency)
- ✅ Position management (bidirectional positions)
- ✅ Account management (margin, P&L)
- ✅ Funding rate calculation
- ✅ Event Sourcing & Deterministic Calculation
- ✅ Microservices Architecture (Matching Service + Trading Service)

### Production Features
- ✅ User authentication & authorization (JWT, API keys)
- ✅ Liquidation engine (risk calculation, forced liquidation)
- ✅ Funding rate management (auto settlement)
- ✅ Market data service (K-line, depth, 24h statistics)
- ✅ API Gateway (routing, authentication, rate limiting)
- ✅ Monitoring system (Prometheus metrics, alerts)
- ✅ Notification service (email, SMS, push)
- ✅ Database manager (multi-database support)
- ✅ RESTful API server (HTTP/1.1, JSON)

### Performance Optimizations
- ✅ Memory pool optimization (+5-10% performance)
- ✅ Lock-free data structures (+10-20% concurrency)
- ✅ SIMD optimization (2-4x batch computation on x86_64)
- ✅ NUMA-aware optimization (multi-core)
- ✅ FPGA acceleration framework (reserved)

### Infrastructure Features
- ✅ Logging system (5-level, file output)
- ✅ Configuration management (INI + environment variables)
- ✅ Metrics collection (Prometheus format)
- ✅ Error handling (custom exception system)
- ✅ Rate limiting (Token bucket algorithm)
- ✅ Health checking (system health monitoring)
- ✅ Optimized persistence (async writing, 3.6x throughput)
- ✅ Graceful shutdown (signal handling)
- ✅ Docker support (multi-stage builds)
- ✅ Kubernetes ready

## 📊 Performance

See [benchmark reports](benchmark_reports/README.md) for detailed performance comparison.

### Performance Benchmarks

**Key Optimizations**:
- Memory pooling for efficient allocation
- Lock-free data structures
- SIMD optimizations (AVX2) - **2-4x acceleration**
- ART (Adaptive Radix Tree) - **better cache locality**
- NUMA awareness
- Hot path optimizations

**Performance Results** (vs Original):
- **ART+SIMD**: +25-45% throughput, -35-55% latency ⭐
- **Optimized V2**: +20-30% throughput, -20-30% latency
- **ART**: +10-20% throughput, -15-25% latency
- **Optimized**: +15-25% throughput, -10-20% latency

### Running Benchmarks

```bash
# Mac platform
./run_mac_benchmarks.sh 50000

# Docker platform
docker compose -f docker-compose.benchmark.yml run --rm benchmark-runner /app/run_all_benchmarks.sh 50000
```

## 🏗️ Architecture

### Version Structure

The project includes 9 optimized versions:

1. **original** - Baseline implementation
2. **optimized** - Memory pool + lock-free structures
3. **optimized_v2** - Hot path optimizations
4. **art** - Adaptive Radix Tree implementation
5. **art_simd** - ART + SIMD optimizations
6. **event_sourcing** - Event sourcing pattern
7. **production_basic** - Full production features
8. **production_fast** - High-performance production version
9. **production_safe** - WAL-based zero data loss version

Each version is self-contained in `versions/` directory.

## 🚀 Quick Start

### Prerequisites

- C++17 compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)
- CMake 3.15+
- Docker (optional, for containerized benchmarks)

### Build

```bash
# Build all versions
./build_all_versions.sh

# Or build specific version
cd versions/original
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

### Run

```bash
# Run matching engine
cd versions/original/build
./original_benchmark 50000
```

## 📁 Project Structure

```
perpetual_exchange/
├── include/core/          # Core headers
├── src/core/              # Core implementations
├── versions/              # Version-specific implementations
│   ├── original/
│   ├── optimized/
│   ├── optimized_v2/
│   ├── art/
│   ├── art_simd/
│   ├── event_sourcing/
│   ├── production_basic/
│   ├── production_fast/
│   └── production_safe/
├── benchmark_reports/      # Performance reports
│   ├── mac/               # Mac platform reports
│   └── docker/            # Docker platform reports
├── docs/                   # Documentation
│   └── archive/           # Archived documents
└── tests/                  # Test suites
```

## 📚 Documentation

- [Architecture](ARCHITECTURE.md) - System architecture design
- [Benchmark Guide](BENCHMARK_GUIDE.md) - Performance testing guide
- [Benchmark Reports](benchmark_reports/README.md) - Performance reports
- [Cross-Platform Report](benchmark_reports/CROSS_PLATFORM_BENCHMARK_REPORT.md) - Mac vs Docker comparison

## 🔧 Configuration

Configuration files use INI format with environment variable support:

```ini
[engine]
threads = 4
queue_size = 10000

[persistence]
async = true
batch_size = 100
```

## 🧪 Testing

```bash
# Run unit tests
cd build && ctest

# Run benchmarks
./run_mac_benchmarks.sh 50000
```

## 📦 Docker

```bash
# Build Docker image
docker compose -f docker-compose.benchmark.yml build

# Run benchmarks
docker compose -f docker-compose.benchmark.yml run --rm benchmark-runner
```

## 🤝 Contributing

Contributions are welcome! Please read our contributing guidelines before submitting PRs.

## 📄 License

[Add your license here]

## 🙏 Acknowledgments

- ART (Adaptive Radix Tree) implementation
- SIMD optimizations
- Lock-free data structures
