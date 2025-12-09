# 共识模块可插拔配置指南

## 概述

共识模块已重构为可插拔设计，支持单节点模式和分布式模式。默认使用单节点模式，适合本地开发和测试。

## 配置模式

### 1. 单节点模式（默认，适合本地测试）

单节点模式禁用共识和复制功能，所有操作都在本地执行，无需多节点环境。

```cpp
// 方式1: 使用工厂方法（推荐）
DistributedEventStoreConfig config = DistributedEventStoreConfig::create_single_node();

// 方式2: 手动配置
DistributedEventStoreConfig config;
config.single_node_mode = true;
config.enable_consensus = false;
config.enable_replication = false;
config.replication_factor = 1;

// 创建存储
DistributedEventStore store(config);
store.initialize("./data");
```

**特点**:
- ✅ 无需多节点环境
- ✅ 适合本地开发和测试
- ✅ 性能最优（无网络开销）
- ✅ 默认模式

### 2. 分布式模式（生产环境）

分布式模式启用共识和复制功能，需要多节点环境。

```cpp
// 使用工厂方法（推荐）
DistributedEventStoreConfig config = DistributedEventStoreConfig::create_distributed(
    1,                    // 当前节点ID
    {2, 3},              // 副本节点列表
    3                     // 复制因子
);

// 创建存储
DistributedEventStore store(config);
store.initialize("./data");
```

**特点**:
- ✅ 支持多节点部署
- ✅ 数据复制和共识
- ✅ 故障容错
- ⚠️ 需要多节点环境

## 配置参数说明

### DistributedEventStoreConfig

```cpp
struct DistributedEventStoreConfig {
    NodeID node_id = 1;                    // 当前节点ID
    std::vector<NodeID> replica_nodes;     // 副本节点列表
    size_t replication_factor = 3;         // 复制因子（副本数）
    bool enable_consensus = false;         // 是否启用共识（默认：false）
    bool enable_replication = false;       // 是否启用复制（默认：false）
    bool single_node_mode = true;          // 单节点模式（默认：true）
};
```

### 参数说明

| 参数 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `node_id` | `NodeID` | 1 | 当前节点的唯一标识 |
| `replica_nodes` | `vector<NodeID>` | `{}` | 其他副本节点的ID列表 |
| `replication_factor` | `size_t` | 3 | 数据复制份数 |
| `enable_consensus` | `bool` | `false` | 是否启用共识协议 |
| `enable_replication` | `bool` | `false` | 是否启用事件复制 |
| `single_node_mode` | `bool` | `true` | 单节点模式开关 |

### 模式自动切换

当 `single_node_mode = true` 时，系统会自动：
- 禁用共识 (`enable_consensus = false`)
- 禁用复制 (`enable_replication = false`)
- 设置复制因子为1 (`replication_factor = 1`)

## 使用示例

### 示例1: 本地测试（单节点模式）

```cpp
#include "core/event_sourcing_advanced.h"

// 创建单节点配置
DistributedEventStoreConfig config = DistributedEventStoreConfig::create_single_node();

// 创建存储
DistributedEventStore store(config);
store.initialize("./test_data");

// 使用存储（所有操作都在本地）
Event event;
// ... 设置事件数据 ...
store.append_event(event);

auto events = store.get_events(1, 100);
SequenceID seq = store.get_consensus_sequence();  // 返回本地序列号
```

### 示例2: 生产环境（分布式模式）

```cpp
#include "core/event_sourcing_advanced.h"

// 创建分布式配置
DistributedEventStoreConfig config = DistributedEventStoreConfig::create_distributed(
    1,        // 当前节点ID
    {2, 3},   // 其他节点ID
    3         // 复制因子
);

// 创建存储
DistributedEventStore store(config);
store.initialize("./data");

// 使用存储（会自动复制和共识）
Event event;
// ... 设置事件数据 ...
store.append_event(event);  // 会自动复制到其他节点

auto events = store.get_events(1, 100);  // 可能查询多个节点
SequenceID seq = store.get_consensus_sequence();  // 获取多数节点共识
```

### 示例3: 在撮合引擎中使用

```cpp
#include "core/matching_engine_event_sourcing.h"
#include "core/event_sourcing_advanced.h"

// 创建单节点事件存储（本地测试）
DistributedEventStoreConfig config = DistributedEventStoreConfig::create_single_node();
auto distributed_store = std::make_unique<DistributedEventStore>(config);
distributed_store->initialize("./event_data");

// 使用本地事件存储创建撮合引擎
EventStore* local_store = distributed_store->get_local_store();
MatchingEngineEventSourcing engine(1, local_store);
engine.initialize("./engine_data");
```

## 测试配置

### 单元测试

所有测试默认使用单节点模式：

```cpp
TEST_F(ConsensusTest, BasicFunctionality) {
    // 使用单节点模式（默认）
    DistributedEventStoreConfig config = DistributedEventStoreConfig::create_single_node();
    DistributedEventStore store(config);
    store.initialize("./test_data");
    
    // 测试功能...
}
```

### 集成测试

集成测试可以测试分布式模式（如果有多节点环境）：

```cpp
TEST_F(ConsensusTest, DistributedMode) {
    // 使用分布式模式
    DistributedEventStoreConfig config = DistributedEventStoreConfig::create_distributed(
        1, {2, 3}, 3);
    DistributedEventStore store(config);
    store.initialize("./test_data");
    
    // 测试分布式功能...
}
```

## 迁移指南

### 从旧版本迁移

如果之前使用了硬编码的分布式配置：

**旧代码**:
```cpp
DistributedEventStoreConfig config;
config.node_id = 1;
config.replica_nodes = {2, 3};
config.replication_factor = 3;
config.enable_consensus = true;
```

**新代码（单节点模式）**:
```cpp
DistributedEventStoreConfig config = DistributedEventStoreConfig::create_single_node();
```

**新代码（分布式模式）**:
```cpp
DistributedEventStoreConfig config = DistributedEventStoreConfig::create_distributed(
    1, {2, 3}, 3);
```

## 性能对比

| 模式 | 延迟 | 吞吐量 | 网络开销 | 适用场景 |
|------|------|--------|----------|----------|
| 单节点模式 | 最低 | 最高 | 无 | 本地测试、开发 |
| 分布式模式 | 较高 | 中等 | 有 | 生产环境 |

## 注意事项

1. **默认模式**: 系统默认使用单节点模式，适合大多数场景
2. **向后兼容**: 旧代码仍然可以工作，但建议使用新的工厂方法
3. **性能**: 单节点模式性能最优，分布式模式有网络开销
4. **测试**: 所有测试默认使用单节点模式，无需多节点环境

## 常见问题

### Q: 如何切换到分布式模式？

A: 使用 `create_distributed()` 工厂方法：
```cpp
DistributedEventStoreConfig config = DistributedEventStoreConfig::create_distributed(
    node_id, replica_nodes, replication_factor);
```

### Q: 单节点模式下共识功能是否可用？

A: 单节点模式下共识被禁用，`get_consensus_sequence()` 返回本地序列号。

### Q: 如何判断当前是单节点还是分布式模式？

A: 检查配置的 `single_node_mode` 字段：
```cpp
if (config.single_node_mode) {
    // 单节点模式
} else {
    // 分布式模式
}
```

### Q: 可以在运行时切换模式吗？

A: 不建议。模式应该在初始化时确定，运行时切换可能导致数据不一致。

## 相关文档

- `tests/functional/CONSENSUS_TEST_README.md` - 共识测试文档
- `include/core/event_sourcing_advanced.h` - 分布式事件存储定义
- `src/core/event_sourcing_advanced.cpp` - 分布式事件存储实现

---

**更新时间**: 2025-01-XX
**状态**: 共识模块已重构为可插拔设计，默认单节点模式


## 概述

共识模块已重构为可插拔设计，支持单节点模式和分布式模式。默认使用单节点模式，适合本地开发和测试。

## 配置模式

### 1. 单节点模式（默认，适合本地测试）

单节点模式禁用共识和复制功能，所有操作都在本地执行，无需多节点环境。

```cpp
// 方式1: 使用工厂方法（推荐）
DistributedEventStoreConfig config = DistributedEventStoreConfig::create_single_node();

// 方式2: 手动配置
DistributedEventStoreConfig config;
config.single_node_mode = true;
config.enable_consensus = false;
config.enable_replication = false;
config.replication_factor = 1;

// 创建存储
DistributedEventStore store(config);
store.initialize("./data");
```

**特点**:
- ✅ 无需多节点环境
- ✅ 适合本地开发和测试
- ✅ 性能最优（无网络开销）
- ✅ 默认模式

### 2. 分布式模式（生产环境）

分布式模式启用共识和复制功能，需要多节点环境。

```cpp
// 使用工厂方法（推荐）
DistributedEventStoreConfig config = DistributedEventStoreConfig::create_distributed(
    1,                    // 当前节点ID
    {2, 3},              // 副本节点列表
    3                     // 复制因子
);

// 创建存储
DistributedEventStore store(config);
store.initialize("./data");
```

**特点**:
- ✅ 支持多节点部署
- ✅ 数据复制和共识
- ✅ 故障容错
- ⚠️ 需要多节点环境

## 配置参数说明

### DistributedEventStoreConfig

```cpp
struct DistributedEventStoreConfig {
    NodeID node_id = 1;                    // 当前节点ID
    std::vector<NodeID> replica_nodes;     // 副本节点列表
    size_t replication_factor = 3;         // 复制因子（副本数）
    bool enable_consensus = false;         // 是否启用共识（默认：false）
    bool enable_replication = false;       // 是否启用复制（默认：false）
    bool single_node_mode = true;          // 单节点模式（默认：true）
};
```

### 参数说明

| 参数 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `node_id` | `NodeID` | 1 | 当前节点的唯一标识 |
| `replica_nodes` | `vector<NodeID>` | `{}` | 其他副本节点的ID列表 |
| `replication_factor` | `size_t` | 3 | 数据复制份数 |
| `enable_consensus` | `bool` | `false` | 是否启用共识协议 |
| `enable_replication` | `bool` | `false` | 是否启用事件复制 |
| `single_node_mode` | `bool` | `true` | 单节点模式开关 |

### 模式自动切换

当 `single_node_mode = true` 时，系统会自动：
- 禁用共识 (`enable_consensus = false`)
- 禁用复制 (`enable_replication = false`)
- 设置复制因子为1 (`replication_factor = 1`)

## 使用示例

### 示例1: 本地测试（单节点模式）

```cpp
#include "core/event_sourcing_advanced.h"

// 创建单节点配置
DistributedEventStoreConfig config = DistributedEventStoreConfig::create_single_node();

// 创建存储
DistributedEventStore store(config);
store.initialize("./test_data");

// 使用存储（所有操作都在本地）
Event event;
// ... 设置事件数据 ...
store.append_event(event);

auto events = store.get_events(1, 100);
SequenceID seq = store.get_consensus_sequence();  // 返回本地序列号
```

### 示例2: 生产环境（分布式模式）

```cpp
#include "core/event_sourcing_advanced.h"

// 创建分布式配置
DistributedEventStoreConfig config = DistributedEventStoreConfig::create_distributed(
    1,        // 当前节点ID
    {2, 3},   // 其他节点ID
    3         // 复制因子
);

// 创建存储
DistributedEventStore store(config);
store.initialize("./data");

// 使用存储（会自动复制和共识）
Event event;
// ... 设置事件数据 ...
store.append_event(event);  // 会自动复制到其他节点

auto events = store.get_events(1, 100);  // 可能查询多个节点
SequenceID seq = store.get_consensus_sequence();  // 获取多数节点共识
```

### 示例3: 在撮合引擎中使用

```cpp
#include "core/matching_engine_event_sourcing.h"
#include "core/event_sourcing_advanced.h"

// 创建单节点事件存储（本地测试）
DistributedEventStoreConfig config = DistributedEventStoreConfig::create_single_node();
auto distributed_store = std::make_unique<DistributedEventStore>(config);
distributed_store->initialize("./event_data");

// 使用本地事件存储创建撮合引擎
EventStore* local_store = distributed_store->get_local_store();
MatchingEngineEventSourcing engine(1, local_store);
engine.initialize("./engine_data");
```

## 测试配置

### 单元测试

所有测试默认使用单节点模式：

```cpp
TEST_F(ConsensusTest, BasicFunctionality) {
    // 使用单节点模式（默认）
    DistributedEventStoreConfig config = DistributedEventStoreConfig::create_single_node();
    DistributedEventStore store(config);
    store.initialize("./test_data");
    
    // 测试功能...
}
```

### 集成测试

集成测试可以测试分布式模式（如果有多节点环境）：

```cpp
TEST_F(ConsensusTest, DistributedMode) {
    // 使用分布式模式
    DistributedEventStoreConfig config = DistributedEventStoreConfig::create_distributed(
        1, {2, 3}, 3);
    DistributedEventStore store(config);
    store.initialize("./test_data");
    
    // 测试分布式功能...
}
```

## 迁移指南

### 从旧版本迁移

如果之前使用了硬编码的分布式配置：

**旧代码**:
```cpp
DistributedEventStoreConfig config;
config.node_id = 1;
config.replica_nodes = {2, 3};
config.replication_factor = 3;
config.enable_consensus = true;
```

**新代码（单节点模式）**:
```cpp
DistributedEventStoreConfig config = DistributedEventStoreConfig::create_single_node();
```

**新代码（分布式模式）**:
```cpp
DistributedEventStoreConfig config = DistributedEventStoreConfig::create_distributed(
    1, {2, 3}, 3);
```

## 性能对比

| 模式 | 延迟 | 吞吐量 | 网络开销 | 适用场景 |
|------|------|--------|----------|----------|
| 单节点模式 | 最低 | 最高 | 无 | 本地测试、开发 |
| 分布式模式 | 较高 | 中等 | 有 | 生产环境 |

## 注意事项

1. **默认模式**: 系统默认使用单节点模式，适合大多数场景
2. **向后兼容**: 旧代码仍然可以工作，但建议使用新的工厂方法
3. **性能**: 单节点模式性能最优，分布式模式有网络开销
4. **测试**: 所有测试默认使用单节点模式，无需多节点环境

## 常见问题

### Q: 如何切换到分布式模式？

A: 使用 `create_distributed()` 工厂方法：
```cpp
DistributedEventStoreConfig config = DistributedEventStoreConfig::create_distributed(
    node_id, replica_nodes, replication_factor);
```

### Q: 单节点模式下共识功能是否可用？

A: 单节点模式下共识被禁用，`get_consensus_sequence()` 返回本地序列号。

### Q: 如何判断当前是单节点还是分布式模式？

A: 检查配置的 `single_node_mode` 字段：
```cpp
if (config.single_node_mode) {
    // 单节点模式
} else {
    // 分布式模式
}
```

### Q: 可以在运行时切换模式吗？

A: 不建议。模式应该在初始化时确定，运行时切换可能导致数据不一致。

## 相关文档

- `tests/functional/CONSENSUS_TEST_README.md` - 共识测试文档
- `include/core/event_sourcing_advanced.h` - 分布式事件存储定义
- `src/core/event_sourcing_advanced.cpp` - 分布式事件存储实现

---

**更新时间**: 2025-01-XX
**状态**: 共识模块已重构为可插拔设计，默认单节点模式

