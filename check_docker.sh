#!/bin/bash
echo "=== Docker环境检查 ==="
echo ""

if command -v docker &> /dev/null; then
    echo "✅ Docker已安装"
    docker --version
    echo ""
    
    if docker ps &> /dev/null; then
        echo "✅ Docker daemon正在运行"
        echo ""
        echo "可以运行测试:"
        echo "  ./run_phase1_docker_simple.sh 8 30 5000"
    else
        echo "❌ Docker daemon未运行"
        echo ""
        echo "请启动Docker Desktop或Docker daemon"
        echo "macOS/Windows: 启动Docker Desktop"
        echo "Linux: sudo systemctl start docker"
    fi
else
    echo "❌ Docker未安装"
    echo "请先安装Docker: https://www.docker.com/get-started"
fi
