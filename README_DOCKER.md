# Docker环境下的SIMD性能测试

本指南说明如何在Docker x86_64环境中运行SIMD优化测试，展示2-4x的性能加速效果。

## 前置要求

- Docker Desktop（支持多平台构建）
- 或Linux系统上的Docker（原生x86_64）

## 快速开始

### 方法1: 使用Docker Compose（推荐）

```bash
# 构建并运行
docker-compose up --build

# 后台运行并查看日志
docker-compose up -d
docker-compose logs -f
```

### 方法2: 使用Docker命令

```bash
# 构建镜像
chmod +x docker-build.sh
./docker-build.sh

# 运行测试
docker run --rm --platform linux/amd64 perpetual-exchange:simd
```

### 方法3: 手动构建

```bash
# 构建x86_64镜像
docker buildx build \
    --platform linux/amd64 \
    --tag perpetual-exchange:simd \
    --load \
    -f Dockerfile .

# 运行容器
docker run --rm --platform linux/amd64 perpetual-exchange:simd
```

## 在ARM Mac上运行x86_64容器

如果你使用的是Apple Silicon Mac，需要启用QEMU来运行x86_64容器：

```bash
# 安装QEMU（如果还没有）
docker run --rm --privileged multiarch/qemu-user-static --reset -p yes

# 然后正常构建和运行
docker buildx build --platform linux/amd64 --tag perpetual-exchange:simd --load -f Dockerfile .
docker run --rm --platform linux/amd64 perpetual-exchange:simd
```

## 测试内容

SIMD benchmark包含以下测试：

1. **价格比较测试**: 批量价格比较（10M次）
2. **数量求和测试**: 批量数量求和（10M次）
3. **PnL计算测试**: 批量盈亏计算（1M个仓位）
4. **撮合引擎测试**: 完整撮合引擎性能（100K订单）

## 预期结果

在x86_64平台上，SIMD优化应该提供：

- **价格比较**: 2-3x加速
- **数量求和**: 2-4x加速
- **PnL计算**: 2-3x加速
- **整体吞吐**: 10-20%提升

## 查看结果

测试结果会直接输出到控制台。如果需要保存：

```bash
docker run --rm --platform linux/amd64 \
    -v $(pwd)/results:/app/results \
    perpetual-exchange:simd > results/simd_benchmark.txt
```

## 故障排除

### 问题1: 构建失败 - "platform not supported"

**解决方案**: 确保Docker Desktop启用了多平台支持，或使用`docker buildx`：

```bash
docker buildx create --use
docker buildx build --platform linux/amd64 ...
```

### 问题2: 运行时错误 - "illegal instruction"

**解决方案**: 确保编译时启用了AVX2支持。检查Dockerfile中的编译选项：

```dockerfile
-DCMAKE_CXX_FLAGS="-march=native -mavx2 -mfma -O3"
```

### 问题3: 性能提升不明显

**可能原因**:
- CPU不支持AVX2（检查`/proc/cpuinfo`）
- 编译器没有启用AVX2
- 测试数据量太小

**解决方案**:
- 检查CPU特性: `grep avx2 /proc/cpuinfo`
- 确认编译选项包含`-mavx2`
- 增加测试数据量

## 验证SIMD是否启用

在容器内运行：

```bash
docker run --rm --platform linux/amd64 -it perpetual-exchange:simd /bin/bash
# 在容器内
./simd_benchmark
# 查看输出中的 "AVX2 Support: Yes"
```

## 性能对比

运行对比测试查看SIMD vs 标量实现的性能差异：

```bash
docker run --rm --platform linux/amd64 perpetual-exchange:simd
```

输出会显示：
- 标量实现耗时
- SIMD实现耗时
- 加速倍数（Speedup）

## 注意事项

1. **平台要求**: SIMD优化仅在x86_64平台有效
2. **CPU要求**: 需要支持AVX2指令集（Intel Haswell+或AMD Excavator+）
3. **编译选项**: 必须使用`-mavx2`编译选项
4. **QEMU性能**: 在ARM Mac上通过QEMU运行x86_64会有性能损失，建议在原生x86_64系统上测试

## 进一步优化

如需进一步优化SIMD性能：

1. 使用`-march=native`让编译器针对当前CPU优化
2. 启用FMA指令: `-mfma`
3. 使用更激进的优化: `-O3 -flto`
4. 考虑使用Intel MKL或其他优化的数学库




本指南说明如何在Docker x86_64环境中运行SIMD优化测试，展示2-4x的性能加速效果。

## 前置要求

- Docker Desktop（支持多平台构建）
- 或Linux系统上的Docker（原生x86_64）

## 快速开始

### 方法1: 使用Docker Compose（推荐）

```bash
# 构建并运行
docker-compose up --build

# 后台运行并查看日志
docker-compose up -d
docker-compose logs -f
```

### 方法2: 使用Docker命令

```bash
# 构建镜像
chmod +x docker-build.sh
./docker-build.sh

# 运行测试
docker run --rm --platform linux/amd64 perpetual-exchange:simd
```

### 方法3: 手动构建

```bash
# 构建x86_64镜像
docker buildx build \
    --platform linux/amd64 \
    --tag perpetual-exchange:simd \
    --load \
    -f Dockerfile .

# 运行容器
docker run --rm --platform linux/amd64 perpetual-exchange:simd
```

## 在ARM Mac上运行x86_64容器

如果你使用的是Apple Silicon Mac，需要启用QEMU来运行x86_64容器：

```bash
# 安装QEMU（如果还没有）
docker run --rm --privileged multiarch/qemu-user-static --reset -p yes

# 然后正常构建和运行
docker buildx build --platform linux/amd64 --tag perpetual-exchange:simd --load -f Dockerfile .
docker run --rm --platform linux/amd64 perpetual-exchange:simd
```

## 测试内容

SIMD benchmark包含以下测试：

1. **价格比较测试**: 批量价格比较（10M次）
2. **数量求和测试**: 批量数量求和（10M次）
3. **PnL计算测试**: 批量盈亏计算（1M个仓位）
4. **撮合引擎测试**: 完整撮合引擎性能（100K订单）

## 预期结果

在x86_64平台上，SIMD优化应该提供：

- **价格比较**: 2-3x加速
- **数量求和**: 2-4x加速
- **PnL计算**: 2-3x加速
- **整体吞吐**: 10-20%提升

## 查看结果

测试结果会直接输出到控制台。如果需要保存：

```bash
docker run --rm --platform linux/amd64 \
    -v $(pwd)/results:/app/results \
    perpetual-exchange:simd > results/simd_benchmark.txt
```

## 故障排除

### 问题1: 构建失败 - "platform not supported"

**解决方案**: 确保Docker Desktop启用了多平台支持，或使用`docker buildx`：

```bash
docker buildx create --use
docker buildx build --platform linux/amd64 ...
```

### 问题2: 运行时错误 - "illegal instruction"

**解决方案**: 确保编译时启用了AVX2支持。检查Dockerfile中的编译选项：

```dockerfile
-DCMAKE_CXX_FLAGS="-march=native -mavx2 -mfma -O3"
```

### 问题3: 性能提升不明显

**可能原因**:
- CPU不支持AVX2（检查`/proc/cpuinfo`）
- 编译器没有启用AVX2
- 测试数据量太小

**解决方案**:
- 检查CPU特性: `grep avx2 /proc/cpuinfo`
- 确认编译选项包含`-mavx2`
- 增加测试数据量

## 验证SIMD是否启用

在容器内运行：

```bash
docker run --rm --platform linux/amd64 -it perpetual-exchange:simd /bin/bash
# 在容器内
./simd_benchmark
# 查看输出中的 "AVX2 Support: Yes"
```

## 性能对比

运行对比测试查看SIMD vs 标量实现的性能差异：

```bash
docker run --rm --platform linux/amd64 perpetual-exchange:simd
```

输出会显示：
- 标量实现耗时
- SIMD实现耗时
- 加速倍数（Speedup）

## 注意事项

1. **平台要求**: SIMD优化仅在x86_64平台有效
2. **CPU要求**: 需要支持AVX2指令集（Intel Haswell+或AMD Excavator+）
3. **编译选项**: 必须使用`-mavx2`编译选项
4. **QEMU性能**: 在ARM Mac上通过QEMU运行x86_64会有性能损失，建议在原生x86_64系统上测试

## 进一步优化

如需进一步优化SIMD性能：

1. 使用`-march=native`让编译器针对当前CPU优化
2. 启用FMA指令: `-mfma`
3. 使用更激进的优化: `-O3 -flto`
4. 考虑使用Intel MKL或其他优化的数学库



