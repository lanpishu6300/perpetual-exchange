# 压测指南

所有版本撮合引擎的性能测试指南。

[English](BENCHMARK_GUIDE.md) | [中文](BENCHMARK_GUIDE.zh-CN.md)

## 快速开始

### Mac平台

```bash
./run_mac_benchmarks.sh 50000
```

报告将生成在 `benchmark_reports/mac/` 目录。

### Docker平台

```bash
docker compose -f docker-compose.benchmark.yml run --rm benchmark-runner /app/run_all_benchmarks.sh 50000
```

报告将生成在 `benchmark_reports/docker/` 目录。

## 查看报告

所有报告位于 `benchmark_reports/` 目录：

- `mac/` - Mac平台报告
- `docker/` - Docker平台报告  
- `CROSS_PLATFORM_BENCHMARK_REPORT.md` - 跨平台对比报告

## 报告格式

每个报告包含：

- 测试概述（版本、日期、订单数）
- 性能指标（吞吐量、延迟）
- 延迟统计（平均、最小、最大、P50/P90/P99）
- 版本特性说明

## 自定义配置

您可以自定义测试订单数：

```bash
# 使用100,000单测试
./run_mac_benchmarks.sh 100000

# Docker使用100,000单
docker compose -f docker-compose.benchmark.yml run --rm benchmark-runner /app/run_all_benchmarks.sh 100000
```

## 故障排除

### 构建问题

如果压测无法构建，请确保：
- 已安装C++17编译器
- CMake 3.15+可用
- 所有依赖已安装

### Docker问题

如果Docker压测失败：
- 确保Docker正在运行
- 检查Docker Compose版本
- 验证平台兼容性（linux/amd64）

