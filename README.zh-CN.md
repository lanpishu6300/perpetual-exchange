# Perpetual Exchange - 高性能撮合引擎

一个生产就绪的永续合约交易所撮合引擎，具有纳秒级延迟，采用内存池、无锁数据结构、SIMD加速和优化持久化等先进优化技术。

[English](README.md) | [中文文档](README.zh-CN.md)

## 🚀 功能特性

### 核心交易功能
- ✅ 订单簿管理（红黑树、ART，O(log n)）
- ✅ 价格时间优先撮合引擎（纳秒级延迟）
- ✅ 持仓管理（双向持仓）
- ✅ 账户管理（保证金、盈亏）
- ✅ 资金费率计算
- ✅ 事件溯源与确定性计算
- ✅ 微服务架构（撮合服务 + 交易服务）

### 生产功能
- ✅ 用户认证与授权（JWT、API密钥）
- ✅ 强平引擎（风险计算、强制平仓）
- ✅ 资金费率管理（自动结算）
- ✅ 行情数据服务（K线、深度、24小时统计）
- ✅ API网关（路由、认证、限流）
- ✅ 监控系统（Prometheus指标、告警）
- ✅ 通知服务（邮件、短信、推送）
- ✅ 数据库管理器（多数据库支持）
- ✅ RESTful API服务器（HTTP/1.1、JSON）

### 性能优化
- ✅ 内存池优化（性能提升5-10%）
- ✅ 无锁数据结构（并发性能提升10-20%）
- ✅ SIMD优化（x86_64平台批量计算加速2-4倍）
- ✅ NUMA感知优化（多核）
- ✅ FPGA加速框架（预留）

### 基础设施功能
- ✅ 日志系统（5级日志，文件输出）
- ✅ 配置管理（INI + 环境变量）
- ✅ 指标收集（Prometheus格式）
- ✅ 错误处理（自定义异常系统）
- ✅ 限流（令牌桶算法）
- ✅ 健康检查（系统健康监控）
- ✅ 优化持久化（异步写入，吞吐量提升3.6倍）
- ✅ 优雅关闭（信号处理）
- ✅ Docker支持（多阶段构建）
- ✅ Kubernetes就绪

## 📊 性能

详细性能对比请参见[压测报告](benchmark_reports/README.md)。

### 性能基准测试

**关键优化**：
- 内存池高效分配
- 无锁数据结构
- SIMD优化（AVX2）- **加速2-4倍**
- ART（自适应基数树）- **更好的缓存局部性**
- NUMA感知
- 热路径优化

**性能结果**（相比原始版本）：
- **ART+SIMD**: 吞吐量提升25-45%，延迟降低35-55% ⭐
- **Optimized V2**: 吞吐量提升20-30%，延迟降低20-30%
- **ART**: 吞吐量提升10-20%，延迟降低15-25%
- **Optimized**: 吞吐量提升15-25%，延迟降低10-20%

### 运行压测

```bash
# Mac平台
./run_mac_benchmarks.sh 50000

# Docker平台
docker compose -f docker-compose.benchmark.yml run --rm benchmark-runner /app/run_all_benchmarks.sh 50000
```

## 🏗️ 架构

### 版本结构

项目包含9个优化版本：

1. **original** - 基线实现
2. **optimized** - 内存池 + 无锁结构
3. **optimized_v2** - 热路径优化
4. **art** - 自适应基数树实现
5. **art_simd** - ART + SIMD优化
6. **event_sourcing** - 事件溯源模式
7. **production_basic** - 完整生产功能
8. **production_fast** - 高性能生产版本
9. **production_safe** - 基于WAL的零数据丢失版本

每个版本在 `versions/` 目录下独立存在。

## 🚀 快速开始

### 前置要求

- C++17兼容编译器（GCC 7+、Clang 5+、MSVC 2017+）
- CMake 3.15+
- Docker（可选，用于容器化压测）

### 构建

```bash
# 构建所有版本
./build_all_versions.sh

# 或构建特定版本
cd versions/original
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

### 运行

```bash
# 运行撮合引擎
cd versions/original/build
./original_benchmark 50000
```

## 📁 项目结构

```
perpetual_exchange/
├── include/core/          # 核心头文件
├── src/core/              # 核心实现
├── versions/              # 版本特定实现
│   ├── original/
│   ├── optimized/
│   ├── optimized_v2/
│   ├── art/
│   ├── art_simd/
│   ├── event_sourcing/
│   ├── production_basic/
│   ├── production_fast/
│   └── production_safe/
├── benchmark_reports/      # 性能报告
│   ├── mac/               # Mac平台报告
│   └── docker/            # Docker平台报告
├── docs/                   # 文档
│   └── archive/           # 归档文档
└── tests/                  # 测试套件
```

## 📚 文档

- [架构设计](ARCHITECTURE.md) - 系统架构设计
- [压测指南](BENCHMARK_GUIDE.md) - 性能测试指南
- [压测报告](benchmark_reports/README.md) - 性能报告
- [跨平台报告](benchmark_reports/CROSS_PLATFORM_BENCHMARK_REPORT.md) - Mac vs Docker对比

## 🔧 配置

配置文件使用INI格式，支持环境变量：

```ini
[engine]
threads = 4
queue_size = 10000

[persistence]
async = true
batch_size = 100
```

## 🧪 测试

```bash
# 运行单元测试
cd build && ctest

# 运行压测
./run_mac_benchmarks.sh 50000
```

## 📦 Docker

```bash
# 构建Docker镜像
docker compose -f docker-compose.benchmark.yml build

# 运行压测
docker compose -f docker-compose.benchmark.yml run --rm benchmark-runner
```

## 🤝 贡献

欢迎贡献！提交PR前请阅读贡献指南。

## 📄 许可证

[添加您的许可证]

## 🙏 致谢

- ART（自适应基数树）实现
- SIMD优化
- 无锁数据结构

