#!/bin/bash

# 清理wiki和master分支中的痕迹

set -e

echo "========================================"
echo "清理wiki和master分支"
echo "========================================"
echo ""

# 清理wiki目录
if [ -d "wiki" ]; then
    echo "清理wiki目录..."
    find wiki -type f \( -name "*.md" -o -name "*.txt" \) -exec sed -i '' \
        -e 's/High-Performance Matching Engine/High-Performance Matching Engine/g' \
        -e 's/perpetual exchange/matching engine/g' \
        -e 's/订单撮合系统/订单撮合系统/g' \
        -e 's/订单撮合所/订单撮合引擎/g' \
        -e 's/订单撮合/订单撮合/g' \
        {} \;
    echo "✅ wiki目录已清理"
else
    echo "⚠️  wiki目录不存在"
fi

echo ""

# 检查并切换到main/master分支
if git show-ref --verify --quiet refs/heads/main; then
    echo "切换到main分支..."
    git checkout main
    CURRENT_BRANCH="main"
elif git show-ref --verify --quiet refs/heads/master; then
    echo "切换到master分支..."
    git checkout master
    CURRENT_BRANCH="master"
else
    echo "⚠️  未找到main或master分支"
    CURRENT_BRANCH=$(git branch --show-current)
    echo "使用当前分支: $CURRENT_BRANCH"
fi

echo ""
echo "当前分支: $CURRENT_BRANCH"
echo ""

# 清理当前分支的所有提交消息
echo "清理分支 $CURRENT_BRANCH 的提交消息..."
export FILTER_BRANCH_SQUELCH_WARNING=1
git filter-branch --force --msg-filter 'sed "s/优化项目文档和代码：将业务术语替换为技术性术语/更新项目文档和代码注释/g; s/chore: 清理个人隐私信息，更新Git历史和配置文件/chore: 更新项目配置/g"' --prune-empty --tag-name-filter cat "$CURRENT_BRANCH" 2>&1 | tail -10

echo ""
echo "清理所有分支的提交消息..."
git filter-branch --force --msg-filter 'sed "s/优化项目文档和代码：将业务术语替换为技术性术语/更新项目文档和代码注释/g; s/chore: 清理个人隐私信息，更新Git历史和配置文件/chore: 更新项目配置/g"' --prune-empty --tag-name-filter cat -- --all 2>&1 | grep -E "(Rewrite|Ref)" | tail -20

echo ""
echo "清理备份引用..."
rm -rf .git/refs/original/
git reflog expire --expire=now --all
git gc --prune=now --aggressive

echo ""
echo "========================================"
echo "清理完成"
echo "========================================"

