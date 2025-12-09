#!/bin/bash

# GitHub Wiki 迁移脚本
# 将设计文档迁移到 GitHub Wiki

REPO_URL="https://github.com/lanpishu6300/perpetual-exchange.git"
WIKI_URL="${REPO_URL%.git}.wiki.git"
WIKI_DIR="/tmp/perpetual-exchange-wiki"
PROJECT_ROOT="/Users/lan/Downloads/perpetual_exchange"

echo "=========================================="
echo "GitHub Wiki 文档迁移工具"
echo "=========================================="
echo ""

# 检查是否已存在 wiki 目录
if [ -d "$WIKI_DIR" ]; then
    echo "Wiki 目录已存在，更新中..."
    cd "$WIKI_DIR"
    git pull
else
    echo "克隆 Wiki 仓库..."
    git clone "$WIKI_URL" "$WIKI_DIR"
    cd "$WIKI_DIR"
fi

echo ""
echo "开始迁移文档..."
echo ""

# 创建 _Sidebar.md
cat > _Sidebar.md << 'SIDEBAR'
# 永续合约交易系统文档

## 核心文档
- [[Home|首页]]
- [[Architecture|系统架构]]
- [[Event-Sourcing-Design|事件溯源设计]]
- [[Microservices-Architecture|微服务架构]]

## 开发指南
- [[Consensus-Pluggable-Guide|共识模块可插拔指南]]
- [[Testing-Guide|测试指南]]
- [[Performance-Optimization-Guide|性能优化指南]]
- [[Performance-Testing-Guide|性能测试指南]]

## 部署与运维
- [[Deployment-Guide|部署指南]]
- [[Production-Integration-Guide|生产环境集成指南]]
- [[Docker-Quick-Start|Docker 快速开始]]

## 设计文档
- [[Trading-Service-CQRS-Design|交易服务CQRS设计]]
SIDEBAR

# 迁移文档（使用兼容的方式）
migrate_doc() {
    local wiki_name=$1
    local doc_path=$2
    local full_path="$PROJECT_ROOT/$doc_path"
    
    if [ -f "$full_path" ]; then
        echo "迁移: $doc_path -> $wiki_name.md"
        cp "$full_path" "${wiki_name}.md"
        
        # 添加返回首页链接
        echo "" >> "${wiki_name}.md"
        echo "---" >> "${wiki_name}.md"
        echo "[返回首页](Home)" >> "${wiki_name}.md"
        return 0
    else
        echo "警告: 文件不存在: $full_path"
        return 1
    fi
}

# 迁移核心文档
migrate_doc "Architecture" "ARCHITECTURE.md"
migrate_doc "Event-Sourcing-Design" "EVENT_SOURCING_CORE_DESIGN.md"
migrate_doc "Microservices-Architecture" "MICROSERVICES_ARCHITECTURE.md"
migrate_doc "Consensus-Pluggable-Guide" "CONSENSUS_PLUGGABLE_GUIDE.md"
migrate_doc "Deployment-Guide" "DEPLOYMENT_GUIDE.md"
migrate_doc "Performance-Optimization-Guide" "PERFORMANCE_OPTIMIZATION_GUIDE.md"
migrate_doc "Performance-Testing-Guide" "PERFORMANCE_TESTING_GUIDE.md"
migrate_doc "Production-Integration-Guide" "PRODUCTION_INTEGRATION_GUIDE.md"
migrate_doc "Docker-Quick-Start" "DOCKER_QUICK_START.md"
migrate_doc "Testing-Guide" "tests/TESTING_GUIDE.md"
migrate_doc "Trading-Service-CQRS-Design" "TRADING_SERVICE_CQRS_DESIGN.md"

# 创建首页
if [ ! -f "Home.md" ]; then
    cat > Home.md << HOME
# 永续合约交易系统

欢迎来到永续合约交易系统文档中心！

## 系统概述

这是一个高性能的永续合约交易系统，支持：
- 事件溯源架构
- 确定性计算
- 微服务架构
- 可插拔共识模块
- 高性能订单匹配引擎

## 快速开始

- [[Docker-Quick-Start|Docker 快速开始]]
- [[Deployment-Guide|部署指南]]

## 核心文档

### 架构设计
- [[Architecture|系统架构]]
- [[Event-Sourcing-Design|事件溯源设计]]
- [[Microservices-Architecture|微服务架构]]

### 开发指南
- [[Testing-Guide|测试指南]]
- [[Performance-Optimization-Guide|性能优化指南]]
- [[Consensus-Pluggable-Guide|共识模块可插拔指南]]

### 部署运维
- [[Deployment-Guide|部署指南]]
- [[Production-Integration-Guide|生产环境集成指南]]

## 版本信息

当前版本: v1.1.1

## 相关链接

- [GitHub 仓库]($REPO_URL)
- [问题反馈](${REPO_URL}/issues)

---
最后更新: $(date)
HOME
fi

echo ""
echo "文档迁移完成！"
echo ""
echo "下一步操作："
echo "1. 检查迁移的文档: cd $WIKI_DIR"
echo "2. 查看变更: git status"
echo "3. 提交变更: git add . && git commit -m 'docs: 迁移设计文档到 Wiki'"
echo "4. 推送到 GitHub: git push"
echo ""
