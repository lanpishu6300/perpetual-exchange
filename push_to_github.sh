#!/bin/bash

# GitHub上传脚本
# 使用方法: ./push_to_github.sh YOUR_GITHUB_USERNAME

set -e

if [ -z "$1" ]; then
    echo "使用方法: ./push_to_github.sh YOUR_GITHUB_USERNAME"
    echo "例如: ./push_to_github.sh lanpishu6300"
    exit 1
fi

GITHUB_USERNAME=$1
REPO_NAME="perpetual-exchange"

echo "=========================================="
echo "准备上传到GitHub"
echo "=========================================="
echo ""
echo "GitHub用户名: $GITHUB_USERNAME"
echo "仓库名称: $REPO_NAME"
echo ""

# 检查是否已有远程仓库
if git remote get-url origin >/dev/null 2>&1; then
    echo "⚠️  已存在远程仓库，将更新URL..."
    git remote set-url origin "https://github.com/$GITHUB_USERNAME/$REPO_NAME.git"
else
    echo "✅ 添加远程仓库..."
    git remote add origin "https://github.com/$GITHUB_USERNAME/$REPO_NAME.git"
fi

# 设置主分支
git branch -M main

echo ""
echo "=========================================="
echo "请确保已在GitHub创建仓库: $REPO_NAME"
echo "=========================================="
echo ""
echo "如果还没有创建，请："
echo "1. 访问 https://github.com/new"
echo "2. 仓库名称: $REPO_NAME"
echo "3. 描述: High-Performance Perpetual Futures Exchange Matching Engine"
echo "4. 选择 Public 或 Private"
echo "5. 不要勾选 'Initialize with README'"
echo "6. 点击 'Create repository'"
echo ""
read -p "按回车键继续推送代码..."

echo ""
echo "正在推送到GitHub..."
echo ""

# 推送代码
if git push -u origin main; then
    echo ""
    echo "=========================================="
    echo "✅ 代码已成功推送到GitHub!"
    echo "=========================================="
    echo ""
    echo "仓库地址: https://github.com/$GITHUB_USERNAME/$REPO_NAME"
    echo ""
else
    echo ""
    echo "=========================================="
    echo "❌ 推送失败"
    echo "=========================================="
    echo ""
    echo "可能的原因："
    echo "1. 仓库尚未创建"
    echo "2. 认证失败（需要使用Personal Access Token）"
    echo ""
    echo "解决方法："
    echo "1. 创建仓库: https://github.com/new"
    echo "2. 使用Personal Access Token作为密码"
    echo "   获取Token: https://github.com/settings/tokens"
    echo ""
    exit 1
fi



