# 版本化代码构建指南

## 快速开始

### 构建单个版本

```bash
# 构建 Original 版本
cd versions/original
mkdir -p build && cd build
cmake ..
make

# 构建 Optimized 版本
cd ../../optimized
mkdir -p build && cd build
cmake ..
make
```

### 统一构建脚本

```bash
# 从项目根目录运行
./build_versions.sh
```

## 目录结构

每个版本目录包含：
```
versions/{version}/
├── include/
│   └── core/          # 版本特定的头文件
├── src/               # 版本特定的源文件
├── CMakeLists.txt     # 版本构建配置
├── README.md          # 版本说明
└── build/             # 构建目录（gitignore）
```

## Include 路径规则

1. **版本特定文件**: `versions/{version}/include/core/xxx.h`
   - 使用 `#include "core/xxx.h"`（CMake会自动添加include目录）

2. **共享基础文件**: `include/core/xxx.h`（项目根目录）
   - 使用 `#include "core/xxx.h"`
   - CMake会自动添加 `${PROJECT_ROOT}/include` 到include路径

## 依赖关系

- **Original**: 无依赖（基础版本）
- **Optimized**: 依赖 Original + 基础工具（memory_pool, lockfree_queue等）
- **ART**: 依赖 Original + ART树实现
- **ART+SIMD**: 依赖 ART + SIMD工具
- **Production**: 依赖 Optimized + 生产功能模块
- **Production V2**: 依赖 ART+SIMD + 生产功能模块
- **Production V3**: 依赖 Production V2 + WAL

## 常见问题

### 编译错误：找不到头文件

确保：
1. 头文件在正确的 `versions/{version}/include/core/` 目录
2. CMakeLists.txt 正确设置了 include_directories
3. include 语句使用 `#include "core/xxx.h"` 格式

### 链接错误：找不到符号

确保：
1. 依赖的版本已经编译
2. CMakeLists.txt 正确链接了依赖库
3. 源文件已添加到 CMakeLists.txt 的源文件列表

## 开发新版本

1. 复制现有版本目录作为模板
2. 修改 CMakeLists.txt 设置版本名称
3. 实现新的匹配引擎类
4. 更新 versions/README.md 添加版本说明
5. 测试编译和功能


## 快速开始

### 构建单个版本

```bash
# 构建 Original 版本
cd versions/original
mkdir -p build && cd build
cmake ..
make

# 构建 Optimized 版本
cd ../../optimized
mkdir -p build && cd build
cmake ..
make
```

### 统一构建脚本

```bash
# 从项目根目录运行
./build_versions.sh
```

## 目录结构

每个版本目录包含：
```
versions/{version}/
├── include/
│   └── core/          # 版本特定的头文件
├── src/               # 版本特定的源文件
├── CMakeLists.txt     # 版本构建配置
├── README.md          # 版本说明
└── build/             # 构建目录（gitignore）
```

## Include 路径规则

1. **版本特定文件**: `versions/{version}/include/core/xxx.h`
   - 使用 `#include "core/xxx.h"`（CMake会自动添加include目录）

2. **共享基础文件**: `include/core/xxx.h`（项目根目录）
   - 使用 `#include "core/xxx.h"`
   - CMake会自动添加 `${PROJECT_ROOT}/include` 到include路径

## 依赖关系

- **Original**: 无依赖（基础版本）
- **Optimized**: 依赖 Original + 基础工具（memory_pool, lockfree_queue等）
- **ART**: 依赖 Original + ART树实现
- **ART+SIMD**: 依赖 ART + SIMD工具
- **Production**: 依赖 Optimized + 生产功能模块
- **Production V2**: 依赖 ART+SIMD + 生产功能模块
- **Production V3**: 依赖 Production V2 + WAL

## 常见问题

### 编译错误：找不到头文件

确保：
1. 头文件在正确的 `versions/{version}/include/core/` 目录
2. CMakeLists.txt 正确设置了 include_directories
3. include 语句使用 `#include "core/xxx.h"` 格式

### 链接错误：找不到符号

确保：
1. 依赖的版本已经编译
2. CMakeLists.txt 正确链接了依赖库
3. 源文件已添加到 CMakeLists.txt 的源文件列表

## 开发新版本

1. 复制现有版本目录作为模板
2. 修改 CMakeLists.txt 设置版本名称
3. 实现新的匹配引擎类
4. 更新 versions/README.md 添加版本说明
5. 测试编译和功能

