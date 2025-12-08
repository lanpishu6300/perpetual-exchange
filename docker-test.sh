#!/bin/bash

# Docker环境性能测试脚本
# 使用方法: ./docker-test.sh [订单数量]

set -e

ORDERS=${1:-1000}
IMAGE_NAME="perpetual-benchmark:latest"

echo "========================================="
echo "Docker环境性能测试"
echo "========================================="
echo "订单数量: $ORDERS"
echo "镜像: $IMAGE_NAME"
echo ""

# 检查Docker是否运行
if ! docker info > /dev/null 2>&1; then
    echo "❌ 错误: Docker daemon未运行"
    echo ""
    echo "请先启动Docker:"
    echo "  macOS: 打开 Docker Desktop"
    echo "  Linux: sudo systemctl start docker"
    exit 1
fi

echo "✅ Docker daemon正在运行"
echo ""

# 构建镜像
echo "🔨 构建Docker镜像..."
docker build -f Dockerfile.test -t $IMAGE_NAME . || {
    echo "❌ 构建失败"
    exit 1
}

echo "✅ 镜像构建完成"
echo ""

# 运行性能测试
echo "🚀 运行性能测试..."
echo "========================================="
docker run --rm --platform linux/amd64 $IMAGE_NAME ./comprehensive_performance_comparison $ORDERS

echo ""
echo "✅ 测试完成!"



