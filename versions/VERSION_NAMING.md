# 版本命名说明

## 📁 版本文件夹命名

为了更清晰地体现每个版本的特点，我们将生产版本文件夹重命名为更具描述性的名称：

### 重命名对照表

| 旧名称 | 新名称 | 特点 | 性能 | 适用场景 |
|--------|--------|------|------|----------|
| `production` | `production_basic` | 基础生产版，功能完整 | ~15K orders/sec | 早期生产环境 |
| `production_v2` | `production_fast` | 高性能版，ART+SIMD优化 | ~450K orders/sec | 高性能测试环境 |
| `production_v3` | `production_safe` | WAL安全版，零数据丢失 | ~102K orders/sec | **生产环境推荐** |

## 🎯 版本特点详解

### 📁 production_basic/ (基础生产版)

- **特点**: 功能完整的生产版本
- **技术栈**: 基于 Optimized 版本 + 完整生产功能
- **性能**: ~15K orders/sec, ~13μs 延迟
- **数据安全**: ⚠️ 基本持久化
- **功能完整性**: ⭐⭐⭐⭐⭐
- **适用场景**: 
  - 早期生产环境
  - 功能验证和测试
  - 需要完整功能的场景

### 📁 production_fast/ (高性能版)

- **特点**: 高性能生产版本，使用 ART+SIMD 优化
- **技术栈**: ART+SIMD + 异步持久化 + 缓存优化
- **性能**: ~450K orders/sec, ~2μs 延迟
- **数据安全**: ⚠️ 异步持久化（可能丢失少量数据）
- **功能完整性**: ⭐⭐⭐⭐⭐
- **适用场景**:
  - 高性能测试环境
  - 对性能要求极高的场景
  - 可以容忍少量数据丢失的场景

### 📁 production_safe/ (WAL安全版)

- **特点**: WAL 保证数据安全的最终生产版本
- **技术栈**: production_fast + WAL (Write-Ahead Log)
- **性能**: ~102K orders/sec, ~9.5μs 延迟
- **数据安全**: ✅✅✅ 零数据丢失保证
- **功能完整性**: ⭐⭐⭐⭐⭐
- **适用场景**:
  - **生产环境推荐**
  - 对数据安全要求极高的场景
  - 金融交易系统
  - 关键业务系统

## 📊 版本对比

| 特性 | production_basic | production_fast | production_safe |
|------|------------------|-----------------|-----------------|
| 吞吐量 | ~15K/s | ~450K/s | ~102K/s |
| 延迟 | ~13μs | ~2μs | ~9.5μs |
| 数据安全 | ⚠️ | ⚠️ | ✅✅✅ |
| 功能完整性 | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ |
| 推荐场景 | 早期生产 | 高性能测试 | **生产推荐** |

## 🔧 使用方式

### 构建单个版本

```bash
# 构建基础生产版
cd versions/production_basic
mkdir -p build && cd build
cmake ..
make

# 构建高性能版
cd versions/production_fast
mkdir -p build && cd build
cmake ..
make

# 构建安全版（推荐）
cd versions/production_safe
mkdir -p build && cd build
cmake ..
make
```

### 在 CMake 中使用

```cmake
# 使用基础生产版
add_subdirectory(versions/production_basic)
target_link_libraries(your_target perpetual_production_basic)

# 使用高性能版
add_subdirectory(versions/production_fast)
target_link_libraries(your_target perpetual_production_fast)

# 使用安全版（推荐）
add_subdirectory(versions/production_safe)
target_link_libraries(your_target perpetual_production_safe)
```

### 构建所有版本

```bash
./build_versions.sh
```

## 📝 更新说明

### 已更新的文件

1. ✅ 文件夹重命名
   - `production/` → `production_basic/`
   - `production_v2/` → `production_fast/`
   - `production_v3/` → `production_safe/`

2. ✅ CMakeLists.txt 更新
   - 项目名称更新
   - 库名称更新
   - 注释说明更新

3. ✅ 构建脚本更新
   - `build_versions.sh` 中的版本列表已更新

4. ✅ 文档更新
   - 所有 `.md` 文件中的版本引用已更新

## 🎓 命名原则

新的命名遵循以下原则：

1. **描述性**: 名称直接体现版本的核心特点
2. **简洁性**: 名称简短易记
3. **一致性**: 命名风格统一（`production_*`）
4. **可读性**: 名称清晰易懂，无需查阅文档即可理解

## 📚 相关文档

- [README.md](README.md) - 版本总览
- [BUILD.md](BUILD.md) - 构建指南
- [VERSION_ORGANIZATION.md](VERSION_ORGANIZATION.md) - 版本组织结构



