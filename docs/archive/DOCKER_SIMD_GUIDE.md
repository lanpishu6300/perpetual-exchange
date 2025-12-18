# Docker x86_64 SIMD性能测试指南

## 🎯 目标

在Docker x86_64环境中运行SIMD优化测试，展示2-4x的性能加速效果。

## 📋 前置要求

1. **Docker Desktop** (支持多平台构建)
   - macOS: Docker Desktop for Mac
   - Windows: Docker Desktop for Windows
   - Linux: Docker Engine + buildx插件

2. **CPU要求** (运行容器的主机)
   - x86_64架构
   - 支持AVX2指令集 (Intel Haswell+ 或 AMD Excavator+)

## 🚀 快速开始

### 步骤1: 构建Docker镜像

```bash
cd /Users/lan/Downloads/perpetual_exchange

# 方法1: 使用构建脚本（推荐）
chmod +x docker-build.sh
./docker-build.sh

# 方法2: 使用docker-compose
docker-compose build

# 方法3: 手动构建
docker buildx build \
    --platform linux/amd64 \
    --tag perpetual-exchange:simd \
    --load \
    -f Dockerfile .
```

### 步骤2: 运行SIMD性能测试

```bash
# 方法1: 使用docker-compose（推荐）
docker-compose up

# 方法2: 直接运行容器
docker run --rm --platform linux/amd64 perpetual-exchange:simd

# 方法3: 交互式运行
docker run --rm --platform linux/amd64 -it perpetual-exchange:simd /bin/bash
# 然后在容器内运行: ./simd_benchmark
```

## 📊 预期测试结果

### SIMD加速效果

| 测试项 | 数据量 | 预期加速 | 说明 |
|--------|--------|---------|------|
| 价格比较 | 10M次 | 2-3x | AVX2批量比较 |
| 数量求和 | 10M次 | 2-4x | AVX2向量加法 |
| PnL计算 | 1M仓位 | 2-3x | AVX2批量计算 |
| 撮合引擎 | 100K订单 | 10-20% | 整体性能提升 |

### 示例输出

```
========================================
SIMD Performance Benchmark (x86_64)
========================================

AVX2 Support: Yes

=== SIMD Price Comparison Test ===
Comparisons: 10000000
Scalar time: 45000 μs
SIMD time:   15000 μs
Speedup:     3.00x
Results match: Yes

=== SIMD Quantity Sum Test ===
Sums: 10000000
Scalar time: 38000 μs
SIMD time:   12000 μs
Speedup:     3.17x
Results match: Yes

=== SIMD PnL Calculation Test ===
Positions: 1000000
Scalar time: 25000 μs
SIMD time:   9000 μs
Speedup:     2.78x
Results match: Yes

=== Matching Engine Performance Test ===
Processed 10000 orders, 7801 trades
...
Total Orders: 100000
Total Trades: 78012
Total Time: 380 ms
Throughput: 263.16 K orders/sec
Avg Latency: 2850 ns
```

## 🔧 在ARM Mac上运行（使用QEMU）

如果你使用的是Apple Silicon Mac，需要通过QEMU模拟x86_64：

```bash
# 1. 安装QEMU支持
docker run --rm --privileged multiarch/qemu-user-static --reset -p yes

# 2. 创建buildx builder（如果还没有）
docker buildx create --name multiarch --use

# 3. 构建x86_64镜像
docker buildx build \
    --platform linux/amd64 \
    --tag perpetual-exchange:simd \
    --load \
    -f Dockerfile .

# 4. 运行测试
docker run --rm --platform linux/amd64 perpetual-exchange:simd
```

**注意**: 通过QEMU运行会有性能损失，建议在原生x86_64系统上测试以获得准确结果。

## 📁 文件说明

### Docker相关文件

- `Dockerfile` - Docker镜像构建文件（x86_64平台）
- `docker-compose.yml` - Docker Compose配置
- `docker-build.sh` - 构建脚本
- `.dockerignore` - Docker忽略文件

### 测试程序

- `src/simd_benchmark.cpp` - SIMD性能测试程序
- `include/core/simd_utils.h` - SIMD工具类（已更新支持x86_64）

## 🔍 验证SIMD是否启用

### 方法1: 检查编译输出

构建时应该看到AVX2相关的编译选项：
```
-DCMAKE_CXX_FLAGS="-march=native -mavx2 -mfma -O3"
```

### 方法2: 运行时检查

程序运行时会输出：
```
AVX2 Support: Yes
```

### 方法3: 检查CPU特性

在容器内运行：
```bash
docker run --rm --platform linux/amd64 -it perpetual-exchange:simd /bin/bash
grep avx2 /proc/cpuinfo
```

应该看到 `avx2` 在flags列表中。

## 🐛 故障排除

### 问题1: 构建失败 - "platform not supported"

**错误信息**:
```
ERROR: failed to solve: platform linux/amd64 not supported
```

**解决方案**:
```bash
# 启用buildx
docker buildx create --use

# 或使用docker-compose
docker-compose build
```

### 问题2: 运行时错误 - "illegal instruction"

**原因**: CPU不支持AVX2或编译选项不正确

**解决方案**:
1. 检查Dockerfile中的编译选项包含 `-mavx2`
2. 确认运行环境支持AVX2
3. 检查 `/proc/cpuinfo` 中的flags

### 问题3: SIMD加速不明显

**可能原因**:
- 测试数据量太小
- 内存带宽成为瓶颈
- 编译器优化已经很好

**解决方案**:
- 增加测试数据量（已在代码中设置为较大值）
- 检查是否真的使用了SIMD路径（查看输出）
- 使用perf工具分析性能瓶颈

### 问题4: QEMU性能很差

**原因**: QEMU模拟x86_64有性能损失

**解决方案**:
- 这是正常的，QEMU会有2-10x的性能损失
- 建议在原生x86_64系统上测试
- 或使用云服务器（AWS EC2 x86_64实例）

## 📈 性能分析

### 使用perf分析（在容器内）

```bash
docker run --rm --platform linux/amd64 \
    --privileged \
    -v /sys/kernel/debug:/sys/kernel/debug \
    -it perpetual-exchange:simd /bin/bash

# 安装perf
apt-get update && apt-get install -y linux-perf

# 运行性能分析
perf record ./simd_benchmark
perf report
```

### 查看汇编代码

```bash
# 在构建时生成汇编代码
docker buildx build \
    --platform linux/amd64 \
    --build-arg CMAKE_CXX_FLAGS="-march=native -mavx2 -mfma -O3 -S" \
    --tag perpetual-exchange:simd \
    --load \
    -f Dockerfile .
```

## 🎓 技术细节

### SIMD优化实现

1. **价格比较**: 使用`_mm256_cmpgt_epi64`批量比较4个价格
2. **数量求和**: 使用`_mm256_add_epi64`批量相加
3. **PnL计算**: 使用AVX2向量运算批量计算

### 编译选项说明

- `-march=native`: 针对当前CPU优化
- `-mavx2`: 启用AVX2指令集
- `-mfma`: 启用FMA（融合乘加）指令
- `-O3`: 最高优化级别

## 📝 测试报告模板

运行测试后，可以生成报告：

```bash
docker run --rm --platform linux/amd64 \
    -v $(pwd)/results:/app/results \
    perpetual-exchange:simd > results/simd_report_$(date +%Y%m%d_%H%M%S).txt
```

## 🔗 相关文档

- `README.md` - 项目总体说明
- `PERFORMANCE_COMPARISON.md` - 性能对比分析
- `ARCHITECTURE.md` - 架构设计文档

## ✅ 检查清单

运行测试前确认：

- [ ] Docker已安装并运行
- [ ] 构建成功完成
- [ ] 容器能正常启动
- [ ] 输出显示 "AVX2 Support: Yes"
- [ ] 看到明显的性能加速（2-4x）

---

**最后更新**: 2024年12月
**测试平台**: Docker x86_64 (linux/amd64)



