# Event Sourcing 和 Deterministic Calculation 设计文档

## 概述

本项目已集成 **Event Sourcing（事件溯源）** 和 **Deterministic Calculation（确定性计算）** 设计，确保交易系统的可追溯性、可重现性和计算确定性。

## Event Sourcing（事件溯源）

### 核心组件

1. **Event（事件）**
   - 所有系统状态变更都通过不可变事件记录
   - 事件类型包括：
     - `ORDER_PLACED`: 订单下单
     - `ORDER_MATCHED`: 订单撮合
     - `ORDER_CANCELLED`: 订单取消
     - `ORDER_REJECTED`: 订单拒绝
     - `TRADE_EXECUTED`: 交易执行
     - `ORDER_PARTIALLY_FILLED`: 订单部分成交
     - `ORDER_FULLY_FILLED`: 订单完全成交

2. **EventStore（事件存储）**
   - 追加式日志（Append-Only Log）存储所有事件
   - 支持事件查询和重放
   - 提供快照（Snapshot）功能用于快速恢复
   - 索引优化：按订单ID和合约ID建立索引

3. **EventPublisher（事件发布器）**
   - 非阻塞异步事件发布
   - 自动生成确定性序列号和时间戳

### 使用示例

```cpp
#include "core/matching_engine_event_sourcing.h"
#include "core/event_sourcing.h"

// 创建带Event Sourcing的撮合引擎
MatchingEngineEventSourcing engine(1);  // instrument_id = 1

// 初始化事件存储
engine.initialize("./event_store");

// 处理订单（自动记录事件）
Order order(1001, 2001, 1, OrderSide::BUY, double_to_price(50000.0), double_to_quantity(1.0));
auto trades = engine.process_order_es(&order);

// 查询订单的所有事件
EventStore* store = engine.get_event_store();
auto events = store->get_order_events(1001);

// 重放事件重建状态
engine.replay_events(0, 1000);
```

## Deterministic Calculation（确定性计算）

### 核心原则

1. **固定点数学**
   - 所有价格和数量使用 `int64_t` 类型（固定点表示）
   - 避免浮点数带来的非确定性

2. **确定性时间戳**
   - 使用序列号生成确定性时间戳
   - 确保相同输入产生相同输出

3. **确定性排序**
   - 价格-时间优先级排序使用确定性算法
   - 排序键：`(price << 64) | sequence_id`

### DeterministicCalculator 功能

```cpp
#include "core/deterministic_calculator.h"

// 价格比较（确定性）
bool can_match = DeterministicCalculator::can_match(
    taker_price, maker_price, is_buy_order);

// 计算撮合价格（价格-时间优先级）
Price match_price = DeterministicCalculator::calculate_match_price(
    taker_price, maker_price);

// 计算交易数量（确定性最小值）
Quantity trade_qty = DeterministicCalculator::calculate_trade_quantity(
    taker_remaining, maker_remaining);

// 计算PnL（使用固定点乘法）
Price pnl = DeterministicCalculator::calculate_pnl(
    entry_price, current_price, position_size, is_long);

// 计算保证金（确定性）
Price margin = DeterministicCalculator::calculate_margin(
    price, quantity, margin_rate_bps);

// 计算资金费率支付（确定性）
Price funding = DeterministicCalculator::calculate_funding_payment(
    position_size, price, funding_rate_bps);

// 确定性时间戳
Timestamp ts = DeterministicCalculator::sequence_to_timestamp(sequence_id);
```

### 确定性计算的优势

1. **可重现性**：相同输入总是产生相同输出
2. **可验证性**：可以通过重放事件验证计算结果
3. **调试友好**：问题可以精确重现
4. **审计合规**：满足金融监管要求

## 集成使用

### MatchingEngineEventSourcing

`MatchingEngineEventSourcing` 是集成了 Event Sourcing 和 Deterministic Calculation 的撮合引擎版本。

**特性：**
- 自动记录所有订单和交易事件
- 使用确定性计算进行撮合
- 支持事件重放重建状态
- 确定性时间戳模式

**API：**

```cpp
// 处理订单（带事件记录）
std::vector<Trade> process_order_es(Order* order);

// 取消订单（带事件记录）
bool cancel_order_es(OrderID order_id, UserID user_id);

// 重放事件
bool replay_events(SequenceID from = 0, SequenceID to = UINT64_MAX);

// 启用/禁用确定性模式
void set_deterministic_mode(bool enabled);
bool deterministic_mode() const;
```

## 事件存储格式

事件以二进制格式存储在 `events.log` 文件中：

```
[length: 4 bytes][serialized_event: variable length]
```

序列化格式（CSV）：
```
event_type,sequence_id,timestamp,instrument_id,event_specific_data...
```

## 性能考虑

1. **异步写入**：事件写入是异步的，不阻塞撮合流程
2. **批量写入**：可以批量写入多个事件
3. **索引优化**：使用内存索引加速查询
4. **快照机制**：定期创建快照，避免全量重放

## 使用场景

1. **审计和合规**：完整记录所有交易事件
2. **问题排查**：通过事件重放重现问题
3. **状态恢复**：从事件日志恢复系统状态
4. **回测系统**：使用历史事件进行回测
5. **多副本一致性**：通过事件重放实现多副本状态一致

## 文件结构

```
include/core/
├── event_sourcing.h              # Event Sourcing 核心定义
├── deterministic_calculator.h    # 确定性计算器
└── matching_engine_event_sourcing.h  # 集成版本撮合引擎

src/core/
├── event_sourcing.cpp
├── deterministic_calculator.cpp
└── matching_engine_event_sourcing.cpp
```

## 高级功能

### 1. 事件压缩 (Event Compression)

定期压缩旧事件以节省存储空间。

```cpp
#include "core/event_sourcing_advanced.h"

EventCompressor compressor(event_store);
compressor.set_strategy(CompressionStrategy::SNAPSHOT_AND_DELETE);
compressor.set_compression_interval(100000);  // 每10万事件压缩一次
compressor.set_retention_sequence(10000);    // 保留最近1万事件

// 手动压缩
compressor.compress_events(50000);

// 后台自动压缩
compressor.start_background_compression();
```

**压缩策略：**
- `SNAPSHOT_ONLY`: 仅创建快照，保留所有事件
- `SNAPSHOT_AND_DELETE`: 创建快照并删除旧事件
- `ARCHIVE`: 归档旧事件到压缩文件

### 2. 分布式事件存储 (Distributed Event Store)

支持多节点分布式存储和复制。

```cpp
DistributedEventStoreConfig config;
config.node_id = 1;
config.replica_nodes = {2, 3, 4};
config.replication_factor = 3;
config.enable_consensus = true;

DistributedEventStore distributed_store(config);
distributed_store.initialize("./data");

// 追加事件（自动复制到其他节点）
Event event;
distributed_store.append_event(event);

// 获取共识序列号
SequenceID consensus_seq = distributed_store.get_consensus_sequence();
```

**特性：**
- 自动复制到多个节点
- 共识协议确保一致性
- 节点可用性检测
- 故障恢复

### 3. 事件流处理 (Event Stream Processing)

实时处理事件流，支持订阅和过滤。

```cpp
EventStreamProcessor processor(event_store);

// 订阅所有订单事件
uint64_t sub_id = processor.subscribe(
    [](const Event& event) {
        std::cout << "Event: " << static_cast<int>(event.type) << std::endl;
    },
    [](const Event& event) {
        // 只处理订单相关事件
        return event.type == EventType::ORDER_PLACED ||
               event.type == EventType::ORDER_MATCHED;
    }
);

// 开始实时处理
processor.start_processing();

// 获取统计信息
auto stats = processor.get_statistics();
std::cout << "Processed: " << stats.events_processed << std::endl;

// 取消订阅
processor.unsubscribe(sub_id);
```

**特性：**
- 实时事件流处理（100Hz）
- 多订阅者支持
- 事件过滤
- 非阻塞处理

### 4. CQRS集成 (Command Query Responsibility Segregation)

命令和查询职责分离，优化读写性能。

```cpp
CQRSManager cqrs(engine, event_store);

// 命令端（写）
Command cmd;
cmd.type = Command::PLACE_ORDER;
cmd.order_id = 1001;
cmd.user_id = 2001;
cmd.instrument_id = 1;
cmd.side = OrderSide::BUY;
cmd.price = double_to_price(50000.0);
cmd.quantity = double_to_quantity(1.0);
cmd.order_type = OrderType::LIMIT;

cqrs.execute_command(cmd);

// 查询端（读）
Query query;
query.type = Query::GET_ORDER;
query.order_id = 1001;

QueryResult result = cqrs.execute_query(query);
if (result.success) {
    // 处理查询结果
}

// 启动缓存更新
cqrs.start_cache_update();
```

**架构：**
- **CommandHandler**: 处理写操作，生成事件
- **QueryHandler**: 处理读操作，使用物化视图
- **Materialized Views**: 缓存查询结果，提高性能

### 5. 事件版本化 (Event Versioning)

支持事件模式演进和版本迁移。

```cpp
EventVersionManager version_manager;

// 注册新版本schema
EventSchema schema_v2;
schema_v2.version = 2;
schema_v2.schema_name = "v2";
schema_v2.fields["new_field"] = "string";
version_manager.register_schema(EventType::ORDER_PLACED, schema_v2);

// 注册迁移函数
version_manager.register_migration(
    EventType::ORDER_PLACED,
    1,  // from version
    2,  // to version
    [](const VersionedEvent& event) -> VersionedEvent {
        VersionedEvent migrated = event;
        // 迁移逻辑
        migrated.version = 2;
        return migrated;
    }
);

// 使用版本化事件存储
VersionedEventStore versioned_store;
versioned_store.set_version_manager(&version_manager);
versioned_store.initialize("./data");

// 获取特定版本的事件
auto events = versioned_store.get_versioned_events(0, 1000, 2);  // 迁移到v2
```

**特性：**
- 事件模式版本管理
- 自动版本迁移
- 向后兼容
- 迁移路径规划

## 使用示例

### 完整集成示例

```cpp
#include "core/event_sourcing_advanced.h"

// 1. 创建分布式事件存储
DistributedEventStoreConfig config;
config.node_id = 1;
config.replica_nodes = {2, 3};
DistributedEventStore store(config);
store.initialize("./data");

// 2. 创建事件压缩器
EventCompressor compressor(store.get_local_store());
compressor.set_strategy(CompressionStrategy::SNAPSHOT_AND_DELETE);
compressor.start_background_compression();

// 3. 创建事件流处理器
EventStreamProcessor processor(store.get_local_store());
processor.subscribe([](const Event& e) {
    // 处理事件
});
processor.start_processing();

// 4. 创建CQRS管理器
MatchingEngineEventSourcing engine(1, store.get_local_store());
CQRSManager cqrs(&engine, store.get_local_store());
cqrs.start_cache_update();

// 5. 使用版本管理
EventVersionManager version_manager;
VersionedEventStore versioned_store;
versioned_store.set_version_manager(&version_manager);
```

## 性能优化建议

1. **事件压缩**：定期压缩，减少存储空间
2. **分布式存储**：提高可用性和性能
3. **流处理**：实时处理，低延迟
4. **CQRS**：读写分离，优化查询性能
5. **版本化**：平滑升级，向后兼容

