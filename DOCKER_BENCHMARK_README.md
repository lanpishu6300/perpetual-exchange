# Docker 环境下 Event Sourcing 性能压测指南

## 快速开始

### 1. 构建 Docker 镜像

```bash
docker-compose -f docker-compose.event_sourcing.yml build
```

### 2. 运行性能测试

```bash
# 使用默认配置 (10,000 事件)
docker-compose -f docker-compose.event_sourcing.yml up

# 或指定测试规模
docker-compose -f docker-compose.event_sourcing.yml run --rm event_sourcing_benchmark ./run_event_sourcing_benchmark.sh 50000
```

### 3. 查看测试结果

```bash
# 查看摘要
cat results/benchmark_*/summary.txt

# 查看详细结果
ls -lh results/benchmark_*/

# 生成完整报告
docker-compose -f docker-compose.event_sourcing.yml run --rm event_sourcing_benchmark \
  python3 generate_performance_report.py results/benchmark_* EVENT_SOURCING_DOCKER_PERFORMANCE_REPORT.md
```

## 测试内容

### 1. Event Sourcing 基本操作测试
- 事件写入性能
- 事件读取性能
- 事件重放性能

### 2. Deterministic Calculation 测试
- 价格比较性能
- 撮合价格计算性能
- PnL计算性能
- 保证金计算性能

### 3. Event Store I/O 测试
- 写入延迟和吞吐量
- 读取延迟和吞吐量
- 索引性能

### 4. 事件流处理测试
- 实时处理延迟
- 订阅者性能
- 过滤开销

### 5. CQRS 性能测试
- 命令执行性能
- 查询执行性能
- 缓存效果

### 6. 事件压缩测试
- 压缩速度
- 压缩比
- 不同策略对比

## 环境配置

### Docker 镜像特性

- **基础镜像**: Ubuntu 22.04 (x86_64)
- **编译器优化**: 
  - `-march=native`: 针对目标CPU优化
  - `-mavx2 -mfma`: AVX2 SIMD指令支持
  - `-O3 -flto`: 最高级别优化和链接时优化
- **运行时**: 最小化运行时依赖

### 性能特性

- **CPU性能**: 接近原生性能 (<1% 开销)
- **I/O性能**: 轻微开销 (<5% 开销)
- **内存**: 容器化开销约 10%
- **网络**: 不涉及网络测试

## 测试结果解读

### 性能指标说明

1. **延迟 (Latency)**
   - 单位: 纳秒 (ns)
   - 指标: P50, P90, P99, 平均, 最小, 最大

2. **吞吐量 (Throughput)**
   - 单位: 操作/秒 (ops/sec)
   - 表示系统处理能力

3. **压缩比 (Compression Ratio)**
   - 单位: 百分比 (%)
   - 表示压缩后节省的空间

### 性能基准

| 操作类型 | 优秀 | 良好 | 可接受 |
|---------|------|------|--------|
| 事件写入 | <200 ns | 200-500 ns | 500-1000 ns |
| 事件读取 | <20 ns | 20-50 ns | 50-100 ns |
| 确定性计算 | <10 ns | 10-50 ns | 50-100 ns |
| 流处理 | <200 ns | 200-500 ns | 500-1000 ns |
| CQRS查询 | <50 ns | 50-100 ns | 100-200 ns |

## 故障排查

### 常见问题

1. **构建失败**
   ```bash
   # 清理并重新构建
   docker-compose -f docker-compose.event_sourcing.yml down
   docker-compose -f docker-compose.event_sourcing.yml build --no-cache
   ```

2. **测试超时**
   ```bash
   # 增加超时时间或减少测试规模
   docker-compose -f docker-compose.event_sourcing.yml run --rm \
     -e TIMEOUT=3600 event_sourcing_benchmark ./run_event_sourcing_benchmark.sh 1000
   ```

3. **结果文件未生成**
   ```bash
   # 检查挂载的卷
   docker-compose -f docker-compose.event_sourcing.yml run --rm \
     event_sourcing_benchmark ls -la /app/results
   ```

### 调试模式

```bash
# 进入容器进行调试
docker-compose -f docker-compose.event_sourcing.yml run --rm \
  event_sourcing_benchmark bash

# 在容器内手动运行测试
./run_event_sourcing_benchmark.sh 1000
```

## 性能优化建议

### Docker 环境优化

1. **资源限制**
   ```yaml
   deploy:
     resources:
       limits:
         cpus: '4'
         memory: 8G
   ```

2. **卷优化**
   - 使用本地SSD存储
   - 避免网络文件系统
   - 使用tmpfs for临时数据

3. **CPU亲和性**
   ```bash
   docker run --cpuset-cpus="0-3" ...
   ```

### 测试参数调优

- **小规模测试**: 1,000 - 10,000 事件 (快速验证)
- **中等规模**: 10,000 - 100,000 事件 (标准测试)
- **大规模测试**: 100,000+ 事件 (压力测试)

## 结果分析

### 性能报告结构

```
results/
└── benchmark_YYYYMMDD_HHMMSS/
    ├── summary.txt                    # 测试摘要
    ├── event_sourcing_basic.txt       # 基本操作测试
    ├── comprehensive_comparison.txt   # 综合对比测试
    ├── deterministic_calc_test.txt    # 确定性计算测试
    ├── event_store_io_test.txt       # I/O性能测试
    ├── event_stream_test.txt          # 流处理测试
    ├── cqrs_test.txt                  # CQRS性能测试
    └── compression_test.txt           # 压缩测试
```

### 报告生成

自动生成完整的Markdown报告：

```bash
docker-compose -f docker-compose.event_sourcing.yml run --rm \
  event_sourcing_benchmark python3 generate_performance_report.py \
  results/benchmark_* EVENT_SOURCING_DOCKER_PERFORMANCE_REPORT.md
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
      - uses: actions/checkout@v2
      - name: Build and Run Benchmark
        run: |
          docker-compose -f docker-compose.event_sourcing.yml build
          docker-compose -f docker-compose.event_sourcing.yml up
      - name: Upload Results
        uses: actions/upload-artifact@v2
        with:
          name: benchmark-results
          path: results/
```

## 参考

- [Event Sourcing 设计文档](./EVENT_SOURCING_AND_DETERMINISTIC_CALCULATION.md)
- [性能报告](./EVENT_SOURCING_PERFORMANCE_REPORT.md)
- [Docker 最佳实践](https://docs.docker.com/develop/dev-best-practices/)

