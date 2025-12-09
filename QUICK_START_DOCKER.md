# Docker SIMD测试 - 快速开始

## 🚀 一键运行

```bash
cd /Users/lan/Downloads/perpetual_exchange

# 构建并运行（推荐）
docker-compose up --build

# 或者使用脚本
chmod +x docker-build.sh
./docker-build.sh
docker run --rm --platform linux/amd64 perpetual-exchange:simd
```

## 📊 查看结果

测试会自动运行并输出SIMD加速效果，预期看到：

- ✅ AVX2 Support: Yes
- ✅ 价格比较: 2-3x加速
- ✅ 数量求和: 2-4x加速  
- ✅ PnL计算: 2-3x加速

## 🔧 ARM Mac用户

如果使用Apple Silicon Mac：

```bash
# 1. 启用QEMU
docker run --rm --privileged multiarch/qemu-user-static --reset -p yes

# 2. 运行测试
docker-compose up --build
```

**注意**: QEMU会有性能损失，建议在x86_64系统上测试。

## 📖 详细文档

查看 `README_DOCKER.md` 或 `DOCKER_SIMD_GUIDE.md` 获取完整说明。




## 🚀 一键运行

```bash
cd /Users/lan/Downloads/perpetual_exchange

# 构建并运行（推荐）
docker-compose up --build

# 或者使用脚本
chmod +x docker-build.sh
./docker-build.sh
docker run --rm --platform linux/amd64 perpetual-exchange:simd
```

## 📊 查看结果

测试会自动运行并输出SIMD加速效果，预期看到：

- ✅ AVX2 Support: Yes
- ✅ 价格比较: 2-3x加速
- ✅ 数量求和: 2-4x加速  
- ✅ PnL计算: 2-3x加速

## 🔧 ARM Mac用户

如果使用Apple Silicon Mac：

```bash
# 1. 启用QEMU
docker run --rm --privileged multiarch/qemu-user-static --reset -p yes

# 2. 运行测试
docker-compose up --build
```

**注意**: QEMU会有性能损失，建议在x86_64系统上测试。

## 📖 详细文档

查看 `README_DOCKER.md` 或 `DOCKER_SIMD_GUIDE.md` 获取完整说明。



