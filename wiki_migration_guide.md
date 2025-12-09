# GitHub Wiki 文档迁移指南

## 概述

本指南帮助你将项目中的设计文档迁移到 GitHub Wiki 模块。

## GitHub Wiki 工作原理

GitHub Wiki 实际上是一个独立的 Git 仓库：
- Wiki 仓库 URL: `https://github.com/用户名/仓库名.wiki.git`
- 本项目的 Wiki URL: `https://github.com/lanpishu6300/matching-engine.wiki.git`

## 快速开始

### 方法1: 使用迁移脚本（推荐）

```bash
# 运行迁移脚本
./migrate_to_wiki.sh
```

脚本会自动：
1. 克隆或更新 Wiki 仓库
2. 迁移核心设计文档
3. 创建导航侧边栏
4. 创建首页

### 方法2: 手动迁移

#### 步骤1: 克隆 Wiki 仓库

```bash
git clone https://github.com/lanpishu6300/matching-engine.wiki.git
cd matching-engine.wiki
```

#### 步骤2: 复制文档

将项目中的 Markdown 文档复制到 Wiki 目录，并重命名为合适的名称：

```bash
# 核心文档
cp ../ARCHITECTURE.md Architecture.md
cp ../EVENT_SOURCING_CORE_DESIGN.md Event-Sourcing-Design.md
cp ../MICROSERVICES_ARCHITECTURE.md Microservices-Architecture.md

# 指南文档
cp ../DEPLOYMENT_GUIDE.md Deployment-Guide.md
cp ../PERFORMANCE_OPTIMIZATION_GUIDE.md Performance-Optimization-Guide.md
cp ../tests/TESTING_GUIDE.md Testing-Guide.md
```

#### 步骤3: 创建导航

创建 `_Sidebar.md` 文件：

```markdown
# 订单撮合系统文档

## 核心文档
- [[Home|首页]]
- [[Architecture|系统架构]]
- [[Event-Sourcing-Design|事件溯源设计]]
```

创建 `Home.md` 作为首页。

#### 步骤4: 提交并推送

```bash
git add .
git commit -m "docs: 迁移设计文档到 Wiki"
git push
```

## 文档组织结构建议

### 核心文档
- `Home.md` - 首页和导航
- `Architecture.md` - 系统架构
- `Event-Sourcing-Design.md` - 事件溯源设计
- `Microservices-Architecture.md` - 微服务架构

### 开发指南
- `Testing-Guide.md` - 测试指南
- `Performance-Optimization-Guide.md` - 性能优化指南
- `Consensus-Pluggable-Guide.md` - 共识模块指南

### 部署运维
- `Deployment-Guide.md` - 部署指南
- `Production-Integration-Guide.md` - 生产环境集成
- `Docker-Quick-Start.md` - Docker 快速开始

## 注意事项

1. **文件命名**: Wiki 使用空格和连字符，建议使用连字符（如 `Event-Sourcing-Design.md`）
2. **内部链接**: 使用 `[[Page-Name|显示文本]]` 格式
3. **图片**: 图片需要单独上传到 Wiki 仓库
4. **权限**: 确保有 Wiki 仓库的写入权限

## 维护建议

1. **定期同步**: 当主仓库文档更新时，同步更新 Wiki
2. **版本控制**: Wiki 也有完整的 Git 历史，可以追踪变更
3. **协作**: 团队成员可以直接在 GitHub 网页上编辑 Wiki

## 相关链接

- [GitHub Wiki 文档](https://docs.github.com/en/communities/documenting-your-project-with-wikis)
- [项目 Wiki](https://github.com/lanpishu6300/matching-engine/wiki)
