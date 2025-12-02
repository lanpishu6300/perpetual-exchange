# Docker 环境测试指南

## 快速开始

### 1. 启动 Docker

**macOS:**
```bash
# 打开 Docker Desktop 应用
open -a Docker
# 等待Docker启动完成
```

**Linux:**
```bash
sudo systemctl start docker
```

### 2. 运行测试

```bash
# 快速测试 (1000 orders)
./docker-test.sh 1000

# 完整测试 (5000 orders)
./docker-test.sh 5000
```

## 手动测试步骤

### 步骤 1: 构建镜像

```bash
# 使用测试专用Dockerfile
docker build -f Dockerfile.test -t perpetual-benchmark:test .
```

### 步骤 2: 运行测试

```bash
# 快速测试 (1000订单)
docker run --rm --platform linux/amd64 \
    perpetual-benchmark:test \
    ./comprehensive_performance_comparison 1000

# 完整测试 (5000订单)
docker run --rm --platform linux/amd64 \
    perpetual-benchmark:test \
    ./comprehensive_performance_comparison 5000
```

### 步骤 3: 查看结果

测试结果会直接输出到控制台。

## 预期性能（Docker环境）

由于Docker的虚拟化开销，性能会略低于原生环境：

| 版本 | 原生性能 | Docker性能（预估） |
|------|---------|-------------------|
| Original | 300K/s | ~250K/s (-17%) |
| ART | 409K/s | ~340K/s (-17%) |
| ART+SIMD | 750K/s | ~550K/s (-27%) |

**注意**: SIMD指令在虚拟环境中性能损失更大

## Docker 镜像说明

### Dockerfile.test
- **用途**: 性能测试专用
- **优化**: `-march=x86-64 -O3`
- **包含**: comprehensive_performance_comparison
- **大小**: ~100MB

### Dockerfile (原有)
- **用途**: SIMD benchmark
- **优化**: `-march=native -mavx2`
- **包含**: 多个benchmark工具

## 故障排查

### 问题 1: Docker daemon未运行
```bash
错误: Cannot connect to the Docker daemon
解决: 启动 Docker Desktop 或 systemctl start docker
```

### 问题 2: 平台架构问题
```bash
错误: exec format error
解决: 添加 --platform linux/amd64
```

### 问题 3: 构建超时
```bash
解决: 增加内存限制
docker build --memory=4g -f Dockerfile.test -t perpetual-benchmark:test .
```

### 问题 4: SIMD指令不支持
```bash
错误: Illegal instruction
解决: 使用兼容性更好的 -march=x86-64 而不是 -march=native
```

## 性能对比测试

### 原生 vs Docker

```bash
# 1. 原生环境测试
cd build
./comprehensive_performance_comparison 3000 > native_results.txt

# 2. Docker环境测试  
./docker-test.sh 3000 > docker_results.txt

# 3. 对比结果
diff native_results.txt docker_results.txt
```

## Docker Compose 测试

如果想使用docker-compose:

```bash
# 修改 docker-compose.yml 使用测试Dockerfile
docker-compose -f docker-compose.yml up simd-benchmark
```

## 优化建议

### 提升Docker性能

1. **增加资源限制**:
   ```bash
   docker run --cpus="4" --memory="4g" ...
   ```

2. **使用主机网络**:
   ```bash
   docker run --network host ...
   ```

3. **挂载共享内存**:
   ```bash
   docker run --shm-size=2g ...
   ```

4. **CPU亲和性**:
   ```bash
   docker run --cpuset-cpus="0-3" ...
   ```

## 结论

Docker环境适合:
- ✅ 功能测试
- ✅ 集成测试
- ✅ 部署验证

不适合:
- ❌ 性能基准测试（有虚拟化开销）
- ❌ 纳秒级延迟测试（不够精确）

**建议**: 性能测试使用原生环境，生产部署可考虑容器化。

