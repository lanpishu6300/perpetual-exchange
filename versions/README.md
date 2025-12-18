# 版本化代码组织

本项目将不同版本的匹配引擎代码组织到独立的模块/文件夹中，便于维护和编译。

## 版本列表

### 1. Original (基础版)
- **位置**: `versions/original/`
- **描述**: 基于红黑树的标准实现，作为性能基准
- **文件**:
  - `matching_engine.h/cpp`
  - `orderbook.h/cpp`
- **性能**: ~300K orders/sec, ~3μs延迟

### 2. Optimized (优化版)
- **位置**: `versions/optimized/`
- **描述**: 内存池和无锁队列优化
- **文件**:
  - `matching_engine_optimized.h/cpp`
  - 依赖: Original版本
- **性能**: ~300K orders/sec

### 3. Optimized V2 (热路径优化)
- **位置**: `versions/optimized_v2/`
- **描述**: 热路径代码优化和分支预测
- **文件**:
  - `matching_engine_optimized_v2.h/cpp`
- **性能**: ~321K orders/sec

### 4. ART (自适应基数树)
- **位置**: `versions/art/`
- **描述**: 使用ART树替代红黑树
- **文件**:
  - `matching_engine_art.h/cpp`
  - `orderbook_art.h/cpp`
  - `art_tree.h/cpp`
- **性能**: ~410K orders/sec, +36%提升

### 5. ART+SIMD (SIMD向量化)
- **位置**: `versions/art_simd/`
- **描述**: ART树 + SIMD指令优化
- **文件**:
  - `matching_engine_art_simd.h/cpp`
  - `orderbook_art_simd.h/cpp`
  - `art_tree_simd.h/cpp`
- **性能**: ~750K orders/sec, +150%提升

### 6. Production (生产版)
- **位置**: `versions/production/`
- **描述**: 包含完整生产功能（日志、持久化、验证等）
- **文件**:
  - `matching_engine_production.h/cpp`
  - 依赖: Optimized版本
- **性能**: ~15K orders/sec（功能完整性优先）

### 7. Production V2 (高性能生产版)
- **位置**: `versions/production_fast/`
- **描述**: 异步持久化的高性能生产版本
- **文件**:
  - `matching_engine_production_fast.h/cpp`
  - 依赖: ART+SIMD版本
- **性能**: ~450K orders/sec

### 8. Production V3 (WAL安全版)
- **位置**: `versions/production_safe/`
- **描述**: WAL保证数据安全的最终生产版本
- **文件**:
  - `matching_engine_production_safe.h/cpp`
  - `wal_simple.h/cpp`
- **性能**: ~102K orders/sec

### 9. Event Sourcing (事件溯源版)
- **位置**: `versions/event_sourcing/`
- **描述**: 支持事件溯源的匹配引擎
- **文件**:
  - `matching_engine_event_sourcing.h/cpp`
  - 依赖: Original版本

## 构建说明

每个版本目录都有自己的 `CMakeLists.txt`，可以独立编译。

主 `CMakeLists.txt` 通过选项控制编译哪个版本。

