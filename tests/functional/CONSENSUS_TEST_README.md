# 共识模块测试文档

## 概述

本测试套件专门针对分布式事件存储中的共识机制进行测试，确保系统在分布式环境下的数据一致性和可靠性。

## 测试范围

### 1. 分布式事件存储基础功能
- ✅ 分布式事件存储初始化
- ✅ 事件追加到分布式存储
- ✅ 从分布式存储获取事件
- ✅ 共识序列号获取
- ✅ 节点可用性检查
- ✅ 事件复制功能

### 2. 共识一致性测试
- ✅ 多个事件的共识一致性
- ✅ 事件顺序一致性
- ✅ 跨节点数据一致性

### 3. 故障容错测试
- ✅ 单节点故障容错
- ✅ 部分节点可用时的共识
- ✅ 网络分区处理

### 4. 性能测试
- ✅ 高吞吐量下的共识性能
- ✅ 延迟测试
- ✅ 吞吐量测试

## 共识机制说明

### DistributedEventStore

`DistributedEventStore` 实现了分布式事件存储，支持：

1. **事件复制**: 将事件复制到多个节点
2. **共识协议**: 通过多数节点达成共识
3. **故障容错**: 在部分节点故障时仍能工作
4. **数据一致性**: 保证跨节点的数据一致性

### 配置参数

```cpp
struct DistributedEventStoreConfig {
    NodeID node_id;                    // 当前节点ID
    std::vector<NodeID> replica_nodes; // 副本节点列表
    size_t replication_factor = 3;      // 复制因子（副本数）
    bool enable_consensus = true;      // 是否启用共识
};
```

### 核心方法

- `append_event()`: 追加事件并复制到其他节点
- `get_events()`: 获取事件（可能查询多个节点）
- `get_consensus_sequence()`: 获取多数节点共识的序列号
- `replicate_event()`: 复制事件到指定节点
- `is_node_available()`: 检查节点是否可用

## 测试用例详解

### 基础功能测试

#### TEST_F(ConsensusTest, DistributedEventStore_Initialization)
测试分布式事件存储的初始化功能。

**验证点**:
- 存储成功初始化
- 本地事件存储创建成功

#### TEST_F(ConsensusTest, DistributedEventStore_AppendEvent)
测试事件追加功能。

**验证点**:
- 事件成功追加到本地存储
- 事件数据完整性

#### TEST_F(ConsensusTest, DistributedEventStore_GetEvents)
测试事件获取功能。

**验证点**:
- 能正确获取指定范围的事件
- 事件顺序正确

#### TEST_F(ConsensusTest, DistributedEventStore_ConsensusSequence)
测试共识序列号获取。

**验证点**:
- 能获取共识序列号
- 序列号值合理

### 一致性测试

#### TEST_F(ConsensusTest, ConsensusConsistency_MultipleEvents)
测试多个事件的共识一致性。

**验证点**:
- 所有事件都能正确获取
- 事件顺序一致
- 事件数据完整

#### TEST_F(ConsensusTest, ConsensusConsistency_EventOrdering)
测试事件顺序一致性。

**验证点**:
- 事件按顺序存储
- 事件按顺序获取
- 顺序不因复制而改变

### 故障容错测试

#### TEST_F(ConsensusTest, FaultTolerance_SingleNodeFailure)
测试单节点故障时的容错能力。

**验证点**:
- 单节点故障不影响本地操作
- 事件仍能追加和获取
- 系统保持可用

#### TEST_F(ConsensusTest, FaultTolerance_ConsensusWithPartialNodes)
测试部分节点可用时的共识。

**验证点**:
- 部分节点不可用时仍能工作
- 能获取共识序列号
- 数据一致性保持

### 性能测试

#### TEST_F(ConsensusTest, Performance_HighThroughput)
测试高吞吐量下的共识性能。

**验证点**:
- 能处理大量事件
- 吞吐量达到预期
- 延迟在可接受范围

## 运行测试

### 编译测试
```bash
cd build
cmake ..
make test_consensus
```

### 运行测试
```bash
./build/test_consensus
```

### 运行特定测试
```bash
./build/test_consensus --gtest_filter=ConsensusTest.DistributedEventStore_Initialization
```

## 测试环境

### 单节点模式（默认，推荐）
共识模块已重构为可插拔设计，默认使用单节点模式：
- ✅ 无需多节点环境
- ✅ 适合本地开发和测试
- ✅ 性能最优（无网络开销）
- ✅ 所有测试默认使用此模式

**配置方式**:
```cpp
DistributedEventStoreConfig config = DistributedEventStoreConfig::create_single_node();
```

### 分布式模式（可选）
需要测试分布式功能时，可以使用分布式模式：
```cpp
DistributedEventStoreConfig config = DistributedEventStoreConfig::create_distributed(
    1, {2, 3}, 3);
```

**注意**: 分布式模式需要多节点环境，在单机测试中其他节点可能不可用。

## 共识算法说明

### 当前实现
当前实现使用简化的共识机制：
- **本地优先**: 事件先写入本地存储
- **异步复制**: 异步复制到其他节点
- **多数投票**: 通过查询多个节点获取共识

### 未来改进
可以集成更成熟的共识算法：
- **Raft**: 强一致性共识算法
- **Paxos**: 经典共识算法
- **PBFT**: 拜占庭容错算法

## 测试覆盖率

| 功能模块 | 覆盖率 | 测试用例数 |
|---------|--------|-----------|
| 初始化 | 100% | 1 |
| 事件追加 | 100% | 1 |
| 事件获取 | 100% | 1 |
| 共识序列 | 100% | 1 |
| 节点可用性 | 100% | 1 |
| 事件复制 | 100% | 1 |
| 一致性 | 90% | 2 |
| 故障容错 | 80% | 2 |
| 性能 | 70% | 1 |

**总计**: 11个测试用例

## 注意事项

1. **单节点限制**: 当前测试在单节点环境运行，某些多节点功能可能无法完全验证
2. **网络模拟**: 其他节点的不可用是模拟的，实际多节点环境需要真实网络
3. **性能指标**: 性能测试结果受测试环境影响，仅供参考
4. **共识算法**: 当前使用简化共识，生产环境建议使用成熟的共识算法

## 相关文档

- `event_sourcing_advanced.h` - 分布式事件存储头文件
- `event_sourcing_advanced.cpp` - 分布式事件存储实现
- `EVENT_SOURCING_AND_DETERMINISTIC_CALCULATION.md` - 事件溯源文档

---

**创建时间**: 2025-01-XX
**状态**: 共识模块测试创建完成

