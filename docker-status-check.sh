#!/bin/bash
echo "检查Docker状态..."
if docker info > /dev/null 2>&1; then
    echo "✅ Docker 正在运行"
    echo ""
    docker version --format '版本: {{.Server.Version}}'
    echo ""
    echo "可以运行测试:"
    echo "  ./docker-test.sh 1000"
else
    echo "❌ Docker 未运行"
    echo ""
    echo "请启动Docker:"
    echo "  macOS: open -a Docker"
    echo "  Linux: sudo systemctl start docker"
fi
