#!/bin/bash

# Git历史清理脚本 - 移除个人隐私信息
# 警告：此操作会重写所有分支的Git历史，不可逆！

set -e

echo "=========================================="
echo "Git历史清理脚本"
echo "=========================================="
echo ""
echo "此脚本将："
echo "1. 重写所有分支的Git历史"
echo "2. 替换所有提交中的作者信息"
echo "3. 更新Git配置"
echo ""
echo "警告：此操作不可逆！"
echo ""

# 检查是否有未提交的更改
if ! git diff-index --quiet HEAD --; then
    echo "检测到未提交的更改，正在暂存..."
    git add -A
    git stash push -m "临时暂存：清理历史前的更改"
    STASHED=true
else
    STASHED=false
fi

# 获取所有分支列表
echo "获取所有分支..."
ALL_BRANCHES=$(git branch -a | grep -E 'remotes/origin|^\s*\*' | sed 's/remotes\/origin\///' | sed 's/^\s*\*\s*//' | sed 's/^\s*//' | grep -v 'HEAD' | sort -u)

echo "找到以下分支："
echo "$ALL_BRANCHES"
echo ""

# 新的作者信息（使用通用信息）
NEW_NAME="Perpetual Exchange"
NEW_EMAIL="noreply@perpetual-exchange.local"

# 备份原始配置
echo "备份Git配置..."
cp .git/config .git/config.backup

# 更新Git配置
echo "更新Git配置..."
git config user.name "$NEW_NAME"
git config user.email "$NEW_EMAIL"

# 使用git filter-branch重写所有分支的历史
echo ""
echo "开始重写Git历史（这可能需要一些时间）..."
echo ""

# 重写所有分支和标签的历史
git filter-branch --force --env-filter '
    # 替换所有作者信息（无论原始值是什么）
    OLD_NAME="$GIT_COMMITTER_NAME"
    OLD_EMAIL="$GIT_COMMITTER_EMAIL"
    
    # 替换提交者信息
    if [ "$GIT_COMMITTER_NAME" = "lan" ] || [ "$GIT_COMMITTER_NAME" = "lanpishu6300" ]; then
        export GIT_COMMITTER_NAME="Perpetual Exchange"
    fi
    if [ "$GIT_COMMITTER_EMAIL" = "lanpishu6300@gmail.com" ]; then
        export GIT_COMMITTER_EMAIL="noreply@perpetual-exchange.local"
    fi
    
    # 替换作者信息
    if [ "$GIT_AUTHOR_NAME" = "lan" ] || [ "$GIT_AUTHOR_NAME" = "lanpishu6300" ]; then
        export GIT_AUTHOR_NAME="Perpetual Exchange"
    fi
    if [ "$GIT_AUTHOR_EMAIL" = "lanpishu6300@gmail.com" ]; then
        export GIT_AUTHOR_EMAIL="noreply@perpetual-exchange.local"
    fi
' --tag-name-filter cat -- --branches --tags

# 清理filter-branch创建的备份
echo ""
echo "清理临时文件..."
git for-each-ref --format='%(refname)' refs/original/ | xargs -n 1 git update-ref -d 2>/dev/null || true

# 清理reflog
echo "清理reflog..."
git reflog expire --expire=now --all
git gc --prune=now --aggressive

# 恢复暂存的更改（如果有）
if [ "$STASHED" = true ]; then
    echo ""
    echo "恢复暂存的更改..."
    git stash pop || true
fi

echo ""
echo "=========================================="
echo "清理完成！"
echo "=========================================="
echo ""
echo "已完成的操作："
echo "1. ✓ 重写了所有分支的Git历史"
echo "2. ✓ 替换了所有提交中的作者信息"
echo "3. ✓ 更新了Git配置"
echo "4. ✓ 清理了临时文件和reflog"
echo ""
echo "新的作者信息："
echo "  名称: $NEW_NAME"
echo "  邮箱: $NEW_EMAIL"
echo ""
echo "注意："
echo "- 如果已推送到远程，需要使用 'git push --force --all' 强制推送"
echo "- 建议先备份远程仓库或创建新仓库"
echo "- 原始配置已备份到 .git/config.backup"
echo ""

