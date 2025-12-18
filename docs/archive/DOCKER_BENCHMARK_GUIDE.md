# Docker Benchmark Guide

本指南介绍如何在Docker环境下运行所有版本的benchmark并生成压测报告。

## 快速开始

### 1. 运行所有版本的benchmark

```bash
./run_docker_benchmarks.sh [订单数量]
```

默认订单数量为50000，可以自定义：

```bash
./run_docker_benchmarks.sh 100000
```

### 2. 使用docker-compose运行

```bash
# 运行所有版本
docker-compose -f docker-compose.benchmark.yml up benchmark-runner

# 运行特定版本（使用profile）
docker-compose -f docker-compose.benchmark.yml --profile single up original-benchmark
```

### 3. 手动运行Docker容器

```bash
# 构建镜像
docker build -f Dockerfile.benchmark -t perpetual-benchmark .

# 运行所有benchmark
docker run --rm \
  -v $(pwd)/benchmark_reports:/app/reports \
  -v $(pwd)/versions:/app/versions:ro \
  perpetual-benchmark /app/run_all_benchmarks.sh 50000
```

## 报告位置

所有benchmark报告保存在 `benchmark_reports/` 目录下：

- `original_BENCHMARK_REPORT.md` - Original版本报告
- `optimized_BENCHMARK_REPORT.md` - Optimized版本报告
- `optimized_v2_BENCHMARK_REPORT.md` - Optimized V2版本报告
- `art_BENCHMARK_REPORT.md` - ART版本报告
- `art_simd_BENCHMARK_REPORT.md` - ART SIMD版本报告
- `event_sourcing_BENCHMARK_REPORT.md` - Event Sourcing版本报告
- `production_basic_BENCHMARK_REPORT.md` - Production Basic版本报告
- `production_fast_BENCHMARK_REPORT.md` - Production Fast版本报告
- `production_safe_BENCHMARK_REPORT.md` - Production Safe版本报告
- `COMPREHENSIVE_BENCHMARK_REPORT.md` - 综合报告

## 版本列表

1. **original** - 基准版本（Red-Black Tree）
2. **optimized** - 优化版本（Memory Pool + Lock-Free）
3. **optimized_v2** - 热路径优化版本
4. **art** - Adaptive Radix Tree版本
5. **art_simd** - ART + SIMD优化版本
6. **event_sourcing** - 事件溯源版本
7. **production_basic** - 生产基础版本
8. **production_fast** - 高性能生产版本
9. **production_safe** - 安全生产版本（WAL）

## 报告内容

每个benchmark报告包含：

- **测试概述**: 版本信息、测试日期、订单数量、耗时
- **性能结果**: 吞吐量、交易数、交易率、错误数
- **延迟统计**: 平均、最小、最大、P50/P90/P99延迟
- **版本特性**: 实现方式、性能目标、使用场景

## 环境要求

- Docker Desktop (macOS/Windows) 或 Docker Engine (Linux)
- 至少 4GB 可用内存
- 至少 10GB 可用磁盘空间

## 故障排除

### 构建失败

如果某个版本的benchmark构建失败，检查：

1. CMakeLists.txt是否存在
2. 依赖的源文件是否存在
3. 查看Docker构建日志：`docker-compose -f docker-compose.benchmark.yml build benchmark-runner`

### 运行失败

如果benchmark运行失败：

1. 检查benchmark二进制文件是否存在
2. 查看容器日志：`docker-compose -f docker-compose.benchmark.yml logs benchmark-runner`
3. 尝试单独运行某个版本

### 报告未生成

如果报告未生成：

1. 检查 `benchmark_reports/` 目录权限
2. 查看容器内文件：`docker-compose -f docker-compose.benchmark.yml run --rm benchmark-runner ls -la /app/reports`
3. 手动运行报告生成：`docker-compose -f docker-compose.benchmark.yml run --rm benchmark-runner /app/generate_benchmark_report.sh`

## 高级用法

### 自定义订单数量

```bash
# 使用100000订单
docker-compose -f docker-compose.benchmark.yml run --rm benchmark-runner /app/run_all_benchmarks.sh 100000
```

### 只运行特定版本

```bash
# 只运行original版本
docker-compose -f docker-compose.benchmark.yml --profile single up original-benchmark
```

### 查看实时输出

```bash
docker-compose -f docker-compose.benchmark.yml up benchmark-runner
```

### 进入容器调试

```bash
docker-compose -f docker-compose.benchmark.yml run --rm benchmark-runner bash
```

## 性能建议

- 对于快速测试，使用10000订单
- 对于完整测试，使用50000-100000订单
- 对于压力测试，使用100000+订单

## 注意事项

1. Docker容器使用Linux/amd64平台，确保与本地架构兼容
2. 某些版本可能需要特定的CPU特性（如SIMD），在容器中可能无法使用
3. 报告生成可能需要一些时间，请耐心等待
4. 如果某个版本构建失败，其他版本仍会继续运行

