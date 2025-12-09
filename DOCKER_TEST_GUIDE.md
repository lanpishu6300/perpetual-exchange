# Phase 1 优化 Docker 测试指南

## 前置要求

1. **Docker已安装**
   ```bash
   docker --version
   ```

2. **Docker daemon正在运行**
   ```bash
   docker ps
   ```
   如果报错，需要启动Docker Desktop或Docker daemon

## 快速开始

### 方式1: 使用docker compose (推荐)

```bash
./run_phase1_docker_test.sh [threads] [duration] [orders/sec]
```

例如:
```bash
./run_phase1_docker_test.sh 8 30 5000
```

### 方式2: 直接使用docker命令

```bash
./run_phase1_docker_simple.sh [threads] [duration] [orders/sec]
```

例如:
```bash
./run_phase1_docker_simple.sh 8 30 5000
```

## 测试参数

- **threads**: 并发线程数 (默认: 16)
- **duration**: 测试持续时间（秒）(默认: 60)
- **orders/sec**: 每线程每秒订单数 (默认: 10000)

## 测试配置示例

### 轻量级测试 (快速验证)
```bash
./run_phase1_docker_simple.sh 4 10 2000
```
- 4线程, 10秒, 2000 orders/sec/thread
- 目标TPS: 8,000

### 标准测试
```bash
./run_phase1_docker_simple.sh 8 30 5000
```
- 8线程, 30秒, 5000 orders/sec/thread
- 目标TPS: 40,000

### 压力测试
```bash
./run_phase1_docker_simple.sh 16 60 10000
```
- 16线程, 60秒, 10000 orders/sec/thread
- 目标TPS: 160,000

## 预期输出

测试完成后会显示：

1. **性能统计**
   - 总订单数
   - 总交易数
   - 吞吐量 (orders/sec)

2. **延迟统计**
   - 平均延迟
   - P50延迟
   - P99延迟

3. **引擎统计**
   - 处理的订单数
   - 执行的交易数
   - 平均撮合延迟

4. **持久化统计**
   - 持久化的订单数
   - 持久化的交易数
   - 批量持久化次数
   - 平均持久化延迟

## 故障排除

### Docker daemon未运行

**macOS/Windows:**
- 启动Docker Desktop

**Linux:**
```bash
sudo systemctl start docker
```

### 构建失败

检查：
1. Docker是否正常运行
2. 网络连接是否正常（需要下载Ubuntu镜像）
3. 磁盘空间是否充足

### 测试运行失败

检查：
1. 编译错误 - 查看Docker构建日志
2. 运行时错误 - 查看容器日志
3. 权限问题 - 确保脚本有执行权限

## 手动运行步骤

如果脚本不工作，可以手动运行：

```bash
# 1. 构建镜像
docker build -f Dockerfile.phase1_test -t phase1_test:latest .

# 2. 运行测试
docker run --rm \
    -e TEST_THREADS=8 \
    -e TEST_DURATION=30 \
    -e TEST_ORDERS_PER_SEC=5000 \
    --shm-size=2g \
    phase1_test:latest \
    sh -c "cd /app/build && ./test_optimized_v3 8 30 5000"
```

## 结果分析

### 关键指标

1. **吞吐量**: 应该达到或接近目标TPS
2. **P99延迟**: 应该 < 10μs (优化后)
3. **持久化延迟**: 应该 < 5μs (优化后)

### 优化效果验证

**优化前 (预期)**:
- 持久化延迟: ~50μs
- 吞吐量: ~100K TPS

**优化后 (预期)**:
- 持久化延迟: ~5μs (10x提升)
- 吞吐量: ~300K TPS (3x提升)

## 下一步

测试通过后，可以：
1. 分析性能瓶颈
2. 继续Phase 2优化 (SIMD, NUMA等)
3. 进行更长时间的压力测试
