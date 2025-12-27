#!/bin/bash

# 彻底清理Git历史脚本 - 无条件替换所有作者信息

set -e

echo "=========================================="
echo "彻底清理Git历史"
echo "=========================================="

# 设置环境变量以抑制警告
export FILTER_BRANCH_SQUELCH_WARNING=1

# 新的作者信息
NEW_NAME="High-Performance Matching Engine"
NEW_EMAIL="noreply@perpetual-exchange.local"

echo "开始无条件替换所有提交的作者信息..."
echo "新作者: $NEW_NAME <$NEW_EMAIL>"
echo ""

# 使用git filter-branch无条件替换所有作者信息
git filter-branch --force --env-filter "
    export GIT_COMMITTER_NAME=\"$NEW_NAME\"
    export GIT_COMMITTER_EMAIL=\"$NEW_EMAIL\"
    export GIT_AUTHOR_NAME=\"$NEW_NAME\"
    export GIT_AUTHOR_EMAIL=\"$NEW_EMAIL\"
" --tag-name-filter cat -- --branches --tags

echo ""
echo "清理临时引用..."
git for-each-ref --format="%(refname)" refs/original/ | xargs -n 1 git update-ref -d 2>/dev/null || true

echo "清理reflog..."
git reflog expire --expire=now --all

echo "执行垃圾回收..."
git gc --prune=now --aggressive

echo ""
echo "=========================================="
echo "清理完成！"
echo "=========================================="



