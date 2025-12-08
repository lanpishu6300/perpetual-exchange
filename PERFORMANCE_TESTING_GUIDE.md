# Event Sourcing 性能测试完整指南

## 概述

本文档提供 Event Sourcing 和 Deterministic Calculation 功能的完整性能测试指南，包括本地测试和 Docker 环境测试。

## 快速开始

### 本地测试（macOS/Linux）

```bash
# 1. 编译项目
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make event_sourcing_benchmark -j4

# 2. 运行测试
./event_sourcing_benchmark 10000

# 3. 查看结果
cat results/benchmark_*/summary.txt
```

### Docker 测试（推荐）

```bash
# 1. 确保 Docker 运行
docker info

# 2. 构建镜像
docker compose -f docker-compose.event_sourcing.yml build

# 3. 运行测试
docker compose -f docker-compose.event_sourcing.yml up

# 4. 查看结果
cat results/benchmark_*/summary.txt

# 5. 生成完整报告
docker compose -f docker-compose.event_sourcing.yml run --rm \
  event_sourcing_benchmark python3 generate_performance_report.py \
  results/benchmark_* EVENT_SOURCING_DOCKER_PERFORMANCE_REPORT.md
```

## 测试内容

### 1. Event Sourcing 基本操作

测试事件写入、读取和重放性能。

**预期结果：**
- 写入延迟: 100-500 ns/event
- 读取延迟: 10-50 ns/event (索引)
- 吞吐量: 500K - 2M events/sec

### 2. Deterministic Calculation

测试确定性计算的性能。

**预期结果：**
- 价格比较: 1-5 ns
- 撮合计算: 2-10 ns
- PnL计算: 10-50 ns
- 吞吐量: 20M - 1B ops/sec

### 3. Event Store I/O

测试事件存储的读写性能。

**预期结果：**
- 写入: 500K - 2M events/sec
- 读取: 1M - 10M events/sec (索引)

### 4. 事件流处理

测试实时事件流处理性能。

**预期结果：**
- 处理延迟: 100-1000 ns
- 吞吐量: 100K - 1M events/sec

### 5. CQRS 性能

测试命令查询职责分离的性能。

**预期结果：**
- 命令: 500K - 2M ops/sec
- 查询: 10M - 100M ops/sec (缓存)

### 6. 事件压缩

测试事件压缩的性能和效果。

**预期结果：**
- 压缩速度: 1K - 10K events/sec
- 压缩比: 50-90%

## 性能基准

### 优秀性能

| 操作 | 延迟 | 吞吐量 |
|------|------|--------|
| 事件写入 | <200 ns | >2M ops/sec |
| 事件读取 | <20 ns | >10M ops/sec |
| 确定性计算 | <10 ns | >100M ops/sec |
| 流处理 | <200 ns | >1M events/sec |
| CQRS查询 | <50 ns | >50M ops/sec |

### 可接受性能

| 操作 | 延迟 | 吞吐量 |
|------|------|--------|
| 事件写入 | 200-500 ns | 500K-2M ops/sec |
| 事件读取 | 20-50 ns | 1M-10M ops/sec |
| 确定性计算 | 10-50 ns | 20M-100M ops/sec |
| 流处理 | 200-500 ns | 100K-1M events/sec |
| CQRS查询 | 50-100 ns | 10M-50M ops/sec |

## 结果分析

### 性能报告位置

- **本地测试报告**: `EVENT_SOURCING_PERFORMANCE_REPORT.md`
- **Docker测试报告**: `EVENT_SOURCING_DOCKER_PERFORMANCE_REPORT.md`
- **测试结果目录**: `results/benchmark_YYYYMMDD_HHMMSS/`

### 报告内容

1. **执行摘要**: 关键性能指标总结
2. **详细结果**: 每个测试的详细数据
3. **性能对比**: 与原始版本对比
4. **优化建议**: 性能优化方向
5. **使用场景**: 推荐和不推荐场景

## 故障排查

### 常见问题

1. **编译错误**
   ```bash
   # 清理并重新编译
   rm -rf build && mkdir build && cd build
   cmake .. -DCMAKE_BUILD_TYPE=Release
   make -j4
   ```

2. **Docker 连接失败**
   ```bash
   # 检查 Docker 状态
   docker info
   
   # 重启 Docker Desktop (macOS)
   # 或重启 Docker 服务 (Linux)
   sudo systemctl restart docker
   ```

3. **测试超时**
   ```bash
   # 减少测试规模
   ./event_sourcing_benchmark 1000
   ```

4. **结果文件未生成**
   ```bash
   # 检查权限
   ls -la results/
   
   # 手动创建目录
   mkdir -p results benchmark_data
   ```

## 持续集成

### GitHub Actions 示例

```yaml
name: Event Sourcing Benchmark

on: [push, pull_request]

jobs:
  benchmark:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      
      - name: Build
        run: |
          mkdir build && cd build
          cmake .. -DCMAKE_BUILD_TYPE=Release
          make event_sourcing_benchmark -j4
      
      - name: Run Benchmark
        run: |
          cd build
          ./event_sourcing_benchmark 10000
      
      - name: Upload Results
        uses: actions/upload-artifact@v3
        with:
          name: benchmark-results
          path: results/
```

## 性能优化建议

### 1. 写入优化

- 使用批量写入
- 启用异步I/O
- 使用SSD存储
- 优化序列化格式

### 2. 读取优化

- 维护内存索引
- 使用缓存
- 批量读取
- 预取机制

### 3. 计算优化

- SIMD指令
- 缓存友好数据结构
- 分支预测优化
- 内联函数

### 4. 流处理优化

- 批量处理
- 并行订阅者
- 无锁队列
- 零拷贝

### 5. CQRS优化

- 读写分离
- 物化视图
- 智能缓存
- 索引优化

## 参考文档

- [Event Sourcing 设计文档](./EVENT_SOURCING_AND_DETERMINISTIC_CALCULATION.md)
- [性能报告（本地）](./EVENT_SOURCING_PERFORMANCE_REPORT.md)
- [性能报告（Docker）](./EVENT_SOURCING_DOCKER_PERFORMANCE_REPORT.md)
- [Docker 测试指南](./DOCKER_BENCHMARK_README.md)

## 联系和支持

如有问题或建议，请：
1. 查看相关文档
2. 检查故障排查部分
3. 提交 Issue 或 Pull Request

---

*最后更新: 2025-12-09*

