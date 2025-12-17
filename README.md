# High-Performance Matching Engine - High-Performance Matching Engine

A production-ready perpetual futures exchange matching engine with nanosecond-level latency, featuring advanced optimizations including memory pooling, lock-free data structures, SIMD acceleration, and optimized persistence.

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

See [PERFORMANCE_BENCHMARK_REPORT.md](PERFORMANCE_BENCHMARK_REPORT.md) for detailed performance comparison.

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
# Quick test (10K orders)
./run_benchmark.sh 10000

# Full test (50K orders)
./run_benchmark.sh 50000

# Or run directly
cd build && ./comprehensive_performance_comparison 10000
```

### Persistence Performance

- **Trade Logging**: 368K trades/sec, 2.71 μs latency
- **Order Logging**: 358K orders/sec, 2.79 μs latency
- **Throughput Improvement**: 3.6-3.7x over original

## 🏗️ Architecture

### Monolithic Architecture
```
perpetual_exchange/
├── include/core/          # Core headers
│   ├── order.h            # Order structure
│   ├── orderbook.h        # Order book (Red-Black Tree, ART)
│   ├── matching_engine.h  # Matching engine
│   ├── auth_manager.h     # Authentication & authorization
│   ├── liquidation_engine.h  # Liquidation system
│   ├── funding_rate_manager.h  # Funding rate management
│   ├── market_data_service.h  # Market data service
│   ├── api_gateway.h      # API gateway
│   ├── monitoring_system.h  # Monitoring system
│   ├── notification_service.h  # Notification service
│   ├── database_manager.h  # Database manager
│   └── rest_api_server.h  # REST API server
├── src/core/              # Core implementations
├── src/                   # Applications and benchmarks
├── services/              # Microservices
│   ├── matching_service/  # Matching Service (gRPC)
│   └── trading_service/   # Trading Service (gRPC)
└── docs/                  # Documentation
```

### Microservices Architecture
```
┌─────────────────┐
│   API Gateway   │
└────────┬────────┘
         │
    ┌────┴────┐
    │         │
┌───▼───┐ ┌──▼──────┐
│Trading│ │Matching │
│Service│ │ Service │
└───┬───┘ └───┬─────┘
    │         │
    └────┬────┘
         │
    ┌────▼───────────────┐
    │ Production         │
    │ Components         │
    │ - Auth             │
    │ - Liquidation      │
    │ - Funding Rate     │
    │ - Market Data      │
    │ - Notification     │
    │ - Database         │
    │ - Monitoring       │
    └────────────────────┘
```

## 🚀 Quick Start

### Prerequisites

- C++17 compiler (GCC 7+, Clang 5+, MSVC 2017+)
- CMake 3.10+
- (Optional) Docker for x86_64 SIMD testing

### Build

```bash
# Clone repository
git clone https://github.com/lanpishu6300/perpetual-exchange.git
cd perpetual-exchange

# Build
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j$(nproc)

# Or use Makefile
make build
```

### Run Production Server

```bash
# Prepare configuration
cp config.ini.example config.ini
# Edit config.ini as needed

# Run
cd build
./production_server ../config.ini
```

### Docker Deployment

```bash
# Build production image
make docker-build

# Run with Docker Compose
docker-compose -f docker-compose.production.yml up -d
```

## 📖 Documentation

- [Architecture Guide](ARCHITECTURE.md) - Detailed architecture design
- [Deployment Guide](DEPLOYMENT_GUIDE.md) - Production deployment instructions
- [Performance Comparison](COMPLETE_COMPARISON.md) - Performance benchmarks
- [Persistence Optimization](PERSISTENCE_OPTIMIZATION.md) - Persistence module optimization

## 🔧 Configuration

See `config.ini.example` for all configuration options:

```ini
# Logging
log.level=INFO
log.file=logs/exchange.log

# Rate Limiting
rate_limit.global_orders_per_second=10000.0
rate_limit.per_user_orders_per_second=1000.0

# Persistence
persistence.enabled=true
persistence.db_path=./data
persistence.buffer_size=10000
persistence.flush_interval_ms=100
```

## 📊 Benchmarks

### Run Benchmarks

```bash
# Use the benchmark script
./run_benchmark.sh 10000

# Or run directly
cd build
./comprehensive_performance_comparison 10000  # All versions comparison
./quick_benchmark      # Quick test (10K orders)
./full_benchmark       # Full benchmark
./persistence_benchmark  # Persistence performance
```

### Performance Results

See [PERFORMANCE_BENCHMARK_REPORT.md](PERFORMANCE_BENCHMARK_REPORT.md) for detailed results.

**Summary**:
- **ART+SIMD**: 625-1160K orders/sec, 0.5-1.0 μs latency ⭐
- **Optimized V2**: 600-1040K orders/sec, 0.8-1.4 μs latency
- **Original**: 500-800K orders/sec, 1.2-2.0 μs latency
- **SIMD Acceleration**: 2-4x on x86_64
- **Persistence Throughput**: 360K+ records/sec

## 🎯 Production Ready

This project includes all production-grade features:

- ✅ Comprehensive logging
- ✅ Configuration management
- ✅ Metrics and monitoring
- ✅ Error handling
- ✅ Rate limiting
- ✅ Health checks
- ✅ Optimized persistence
- ✅ Graceful shutdown
- ✅ Docker support

## 📝 License

[Add your license here]

## 👤 Author

lanpishu6300@gmail.com

## 🙏 Acknowledgments

- Inspired by industry-leading nanosecond-latency matching engines
- Built with modern C++17 and best practices

A production-ready perpetual futures exchange matching engine with nanosecond-level latency, featuring advanced optimizations including memory pooling, lock-free data structures, SIMD acceleration, and optimized persistence.

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

See [PERFORMANCE_BENCHMARK_REPORT.md](PERFORMANCE_BENCHMARK_REPORT.md) for detailed performance comparison.

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
# Quick test (10K orders)
./run_benchmark.sh 10000

# Full test (50K orders)
./run_benchmark.sh 50000

# Or run directly
cd build && ./comprehensive_performance_comparison 10000
```

### Persistence Performance

- **Trade Logging**: 368K trades/sec, 2.71 μs latency
- **Order Logging**: 358K orders/sec, 2.79 μs latency
- **Throughput Improvement**: 3.6-3.7x over original

## 🏗️ Architecture

### Monolithic Architecture
```
perpetual_exchange/
├── include/core/          # Core headers
│   ├── order.h            # Order structure
│   ├── orderbook.h        # Order book (Red-Black Tree, ART)
│   ├── matching_engine.h  # Matching engine
│   ├── auth_manager.h     # Authentication & authorization
│   ├── liquidation_engine.h  # Liquidation system
│   ├── funding_rate_manager.h  # Funding rate management
│   ├── market_data_service.h  # Market data service
│   ├── api_gateway.h      # API gateway
│   ├── monitoring_system.h  # Monitoring system
│   ├── notification_service.h  # Notification service
│   ├── database_manager.h  # Database manager
│   └── rest_api_server.h  # REST API server
├── src/core/              # Core implementations
├── src/                   # Applications and benchmarks
├── services/              # Microservices
│   ├── matching_service/  # Matching Service (gRPC)
│   └── trading_service/   # Trading Service (gRPC)
└── docs/                  # Documentation
```

### Microservices Architecture
```
┌─────────────────┐
│   API Gateway   │
└────────┬────────┘
         │
    ┌────┴────┐
    │         │
┌───▼───┐ ┌──▼──────┐
│Trading│ │Matching │
│Service│ │ Service │
└───┬───┘ └───┬─────┘
    │         │
    └────┬────┘
         │
    ┌────▼───────────────┐
    │ Production         │
    │ Components         │
    │ - Auth             │
    │ - Liquidation      │
    │ - Funding Rate     │
    │ - Market Data      │
    │ - Notification     │
    │ - Database         │
    │ - Monitoring       │
    └────────────────────┘
```

## 🚀 Quick Start

### Prerequisites

- C++17 compiler (GCC 7+, Clang 5+, MSVC 2017+)
- CMake 3.10+
- (Optional) Docker for x86_64 SIMD testing

### Build

```bash
# Clone repository
git clone https://github.com/lanpishu6300/perpetual-exchange.git
cd perpetual-exchange

# Build
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j$(nproc)

# Or use Makefile
make build
```

### Run Production Server

```bash
# Prepare configuration
cp config.ini.example config.ini
# Edit config.ini as needed

# Run
cd build
./production_server ../config.ini
```

### Docker Deployment

```bash
# Build production image
make docker-build

# Run with Docker Compose
docker-compose -f docker-compose.production.yml up -d
```

## 📖 Documentation

- [Architecture Guide](ARCHITECTURE.md) - Detailed architecture design
- [Deployment Guide](DEPLOYMENT_GUIDE.md) - Production deployment instructions
- [Performance Comparison](COMPLETE_COMPARISON.md) - Performance benchmarks
- [Persistence Optimization](PERSISTENCE_OPTIMIZATION.md) - Persistence module optimization

## 🔧 Configuration

See `config.ini.example` for all configuration options:

```ini
# Logging
log.level=INFO
log.file=logs/exchange.log

# Rate Limiting
rate_limit.global_orders_per_second=10000.0
rate_limit.per_user_orders_per_second=1000.0

# Persistence
persistence.enabled=true
persistence.db_path=./data
persistence.buffer_size=10000
persistence.flush_interval_ms=100
```

## 📊 Benchmarks

### Run Benchmarks

```bash
# Use the benchmark script
./run_benchmark.sh 10000

# Or run directly
cd build
./comprehensive_performance_comparison 10000  # All versions comparison
./quick_benchmark      # Quick test (10K orders)
./full_benchmark       # Full benchmark
./persistence_benchmark  # Persistence performance
```

### Performance Results

See [PERFORMANCE_BENCHMARK_REPORT.md](PERFORMANCE_BENCHMARK_REPORT.md) for detailed results.

**Summary**:
- **ART+SIMD**: 625-1160K orders/sec, 0.5-1.0 μs latency ⭐
- **Optimized V2**: 600-1040K orders/sec, 0.8-1.4 μs latency
- **Original**: 500-800K orders/sec, 1.2-2.0 μs latency
- **SIMD Acceleration**: 2-4x on x86_64
- **Persistence Throughput**: 360K+ records/sec

## 🎯 Production Ready

This project includes all production-grade features:

- ✅ Comprehensive logging
- ✅ Configuration management
- ✅ Metrics and monitoring
- ✅ Error handling
- ✅ Rate limiting
- ✅ Health checks
- ✅ Optimized persistence
- ✅ Graceful shutdown
- ✅ Docker support

## 📝 License

[Add your license here]

## 👤 Author

lanpishu6300@gmail.com

## 🙏 Acknowledgments

- Inspired by industry-leading nanosecond-latency matching engines
- Built with modern C++17 and best practices