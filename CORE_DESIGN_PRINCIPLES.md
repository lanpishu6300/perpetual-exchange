# 核心设计原则

## Event Sourcing + Deterministic Calculation

**核心功能设计基于 Event Sourcing（事件溯源）和 Deterministic Calculation（确定性计算）**

### 为什么选择这个设计？

#### 1. Event Sourcing（事件溯源）

**定义**: 将所有状态变更作为事件序列存储，通过重放事件重建状态。

**优势**:
- ✅ **完整审计**: 记录所有操作历史
- ✅ **状态重建**: 可以从任意时间点重建状态
- ✅ **调试友好**: 可以重放事件定位问题
- ✅ **时间旅行**: 查询任意历史时刻的状态
- ✅ **分布式友好**: 事件可以跨服务复制

**实现**:
- `EventStore`: 追加式事件日志
- `EventPublisher`: 异步事件发布
- `MatchingEngineEventSourcing`: 集成Event Sourcing的撮合引擎

#### 2. Deterministic Calculation（确定性计算）

**定义**: 所有计算结果完全确定，不依赖外部状态（如系统时间、随机数）。

**优势**:
- ✅ **可重现**: 相同输入产生相同输出
- ✅ **可验证**: 可以验证计算结果正确性
- ✅ **分布式一致性**: 多节点计算结果一致
- ✅ **调试友好**: 可以复现问题场景
- ✅ **测试友好**: 单元测试可预测

**实现**:
- `DeterministicCalculator`: 确定性计算器
- 固定点运算（避免浮点误差）
- 确定性时间戳生成
- 确定性价格/数量比较

### 核心架构

```
┌─────────────────────────────────────────┐
│   MatchingEngineEventSourcing           │
│   (核心撮合引擎)                         │
│                                         │
│   ┌───────────────────────────────┐   │
│   │  Event Sourcing               │   │
│   │  - 记录所有状态变更            │   │
│   │  - 事件持久化                 │   │
│   │  - 事件重放                   │   │
│   └───────────────────────────────┘   │
│                                         │
│   ┌───────────────────────────────┐   │
│   │  Deterministic Calculation    │   │
│   │  - 确定性价格比较              │   │
│   │  - 确定性撮合价格              │   │
│   │  - 确定性时间戳                │   │
│   └───────────────────────────────┘   │
└─────────────────────────────────────────┘
```

### 工作流程

#### 订单处理流程

```
1. 接收订单
   ↓
2. 验证订单（使用确定性验证）
   ↓
3. 发出事件: OrderPlacedEvent
   ↓
4. 确定性撮合（使用DeterministicCalculator）
   ↓
5. 如果成交，发出事件: TradeExecutedEvent
   ↓
6. 更新订单状态，发出事件: OrderUpdatedEvent
   ↓
7. 所有事件存储到EventStore
   ↓
8. 事件发布到EventPublisher（异步）
```

### 确定性计算示例

```cpp
// 传统方式（非确定性）
Price match_price = std::max(buy_order->price, sell_order->price);  // 依赖时间

// 确定性方式
Price match_price = DeterministicCalculator::calculateMatchPrice(
    buy_order->price, 
    sell_order->price,
    buy_order->sequence_id,  // 使用序列ID而不是时间
    sell_order->sequence_id
);
```

### Event Sourcing示例

```cpp
// 处理订单时自动记录事件
auto trades = matching_engine.process_order_es(order);

// 内部会自动发出：
// 1. OrderPlacedEvent
// 2. TradeExecutedEvent (如果有成交)
// 3. OrderUpdatedEvent

// 所有事件都存储在EventStore中
```

### 事件类型

```cpp
enum class EventType {
    ORDER_PLACED,        // 订单提交
    ORDER_CANCELLED,     // 订单取消
    ORDER_REJECTED,      // 订单拒绝
    TRADE_EXECUTED,      // 交易执行
    POSITION_OPENED,     // 开仓
    POSITION_CLOSED,     // 平仓
    FUNDING_SETTLED,     // 资金费率结算
    LIQUIDATION          // 强制平仓
};
```

### 状态重建

```cpp
// 从事件重放重建状态
EventStore event_store;
event_store.initialize("data/event_store");

// 重放所有事件
matching_engine.replay_events(0, UINT64_MAX);

// 或重放特定范围的事件
matching_engine.replay_events(start_sequence, end_sequence);
```

### 优势总结

#### 对交易系统的重要性

1. **合规性**: 完整的事件历史满足监管要求
2. **审计**: 可以追踪每一笔交易的完整历史
3. **故障恢复**: 可以从事件日志完全恢复状态
4. **一致性**: 确定性计算保证多节点结果一致
5. **可验证性**: 可以验证计算结果的正确性

### 与其他组件的关系

#### 微服务架构

```
Matching Service (Event Sourcing)
    ↓ 事件
Event Store Service
    ↓ 事件流
Trading Service (订阅事件)
Account Service (订阅事件)
Position Service (订阅事件)
```

#### 生产组件集成

- **清算系统**: 基于事件触发的清算
- **资金费率**: 基于事件计算的费率
- **市场数据**: 基于事件生成的市场数据
- **通知系统**: 基于事件的通知
- **数据库**: 事件作为数据源

### 性能考虑

#### Event Sourcing开销

- **写入延迟**: +100-300ns（可接受）
- **存储空间**: +20-50%（完整历史）
- **查询性能**: 通过快照优化

#### 确定性计算开销

- **计算延迟**: 几乎无开销（相同复杂度）
- **内存开销**: 无额外开销
- **CPU开销**: 无额外开销

### 使用建议

#### 开发环境

```cpp
// 使用Event Sourcing版本
MatchingEngineEventSourcing engine(instrument_id);
engine.initialize("./event_store");
engine.set_deterministic_mode(true);
```

#### 生产环境

```cpp
// 确保启用Event Sourcing和确定性计算
MatchingEngineEventSourcing engine(instrument_id);
engine.initialize("/data/event_store");
engine.set_deterministic_mode(true);

// 定期创建快照以提高查询性能
// 定期压缩旧事件以节省空间
```

### 总结

**核心功能设计：Event Sourcing + Deterministic Calculation**

这是系统的**核心设计原则**，所有关键组件都围绕这个设计：
- ✅ 完整的事件历史记录
- ✅ 确定性、可重现的计算
- ✅ 可审计、可验证的交易流程
- ✅ 分布式一致性保证

这确保了系统的：
- **可靠性**: 可以从事件重建状态
- **可审计性**: 完整的事件历史
- **一致性**: 确定性计算结果
- **可调试性**: 可以重放事件定位问题


## Event Sourcing + Deterministic Calculation

**核心功能设计基于 Event Sourcing（事件溯源）和 Deterministic Calculation（确定性计算）**

### 为什么选择这个设计？

#### 1. Event Sourcing（事件溯源）

**定义**: 将所有状态变更作为事件序列存储，通过重放事件重建状态。

**优势**:
- ✅ **完整审计**: 记录所有操作历史
- ✅ **状态重建**: 可以从任意时间点重建状态
- ✅ **调试友好**: 可以重放事件定位问题
- ✅ **时间旅行**: 查询任意历史时刻的状态
- ✅ **分布式友好**: 事件可以跨服务复制

**实现**:
- `EventStore`: 追加式事件日志
- `EventPublisher`: 异步事件发布
- `MatchingEngineEventSourcing`: 集成Event Sourcing的撮合引擎

#### 2. Deterministic Calculation（确定性计算）

**定义**: 所有计算结果完全确定，不依赖外部状态（如系统时间、随机数）。

**优势**:
- ✅ **可重现**: 相同输入产生相同输出
- ✅ **可验证**: 可以验证计算结果正确性
- ✅ **分布式一致性**: 多节点计算结果一致
- ✅ **调试友好**: 可以复现问题场景
- ✅ **测试友好**: 单元测试可预测

**实现**:
- `DeterministicCalculator`: 确定性计算器
- 固定点运算（避免浮点误差）
- 确定性时间戳生成
- 确定性价格/数量比较

### 核心架构

```
┌─────────────────────────────────────────┐
│   MatchingEngineEventSourcing           │
│   (核心撮合引擎)                         │
│                                         │
│   ┌───────────────────────────────┐   │
│   │  Event Sourcing               │   │
│   │  - 记录所有状态变更            │   │
│   │  - 事件持久化                 │   │
│   │  - 事件重放                   │   │
│   └───────────────────────────────┘   │
│                                         │
│   ┌───────────────────────────────┐   │
│   │  Deterministic Calculation    │   │
│   │  - 确定性价格比较              │   │
│   │  - 确定性撮合价格              │   │
│   │  - 确定性时间戳                │   │
│   └───────────────────────────────┘   │
└─────────────────────────────────────────┘
```

### 工作流程

#### 订单处理流程

```
1. 接收订单
   ↓
2. 验证订单（使用确定性验证）
   ↓
3. 发出事件: OrderPlacedEvent
   ↓
4. 确定性撮合（使用DeterministicCalculator）
   ↓
5. 如果成交，发出事件: TradeExecutedEvent
   ↓
6. 更新订单状态，发出事件: OrderUpdatedEvent
   ↓
7. 所有事件存储到EventStore
   ↓
8. 事件发布到EventPublisher（异步）
```

### 确定性计算示例

```cpp
// 传统方式（非确定性）
Price match_price = std::max(buy_order->price, sell_order->price);  // 依赖时间

// 确定性方式
Price match_price = DeterministicCalculator::calculateMatchPrice(
    buy_order->price, 
    sell_order->price,
    buy_order->sequence_id,  // 使用序列ID而不是时间
    sell_order->sequence_id
);
```

### Event Sourcing示例

```cpp
// 处理订单时自动记录事件
auto trades = matching_engine.process_order_es(order);

// 内部会自动发出：
// 1. OrderPlacedEvent
// 2. TradeExecutedEvent (如果有成交)
// 3. OrderUpdatedEvent

// 所有事件都存储在EventStore中
```

### 事件类型

```cpp
enum class EventType {
    ORDER_PLACED,        // 订单提交
    ORDER_CANCELLED,     // 订单取消
    ORDER_REJECTED,      // 订单拒绝
    TRADE_EXECUTED,      // 交易执行
    POSITION_OPENED,     // 开仓
    POSITION_CLOSED,     // 平仓
    FUNDING_SETTLED,     // 资金费率结算
    LIQUIDATION          // 强制平仓
};
```

### 状态重建

```cpp
// 从事件重放重建状态
EventStore event_store;
event_store.initialize("data/event_store");

// 重放所有事件
matching_engine.replay_events(0, UINT64_MAX);

// 或重放特定范围的事件
matching_engine.replay_events(start_sequence, end_sequence);
```

### 优势总结

#### 对交易系统的重要性

1. **合规性**: 完整的事件历史满足监管要求
2. **审计**: 可以追踪每一笔交易的完整历史
3. **故障恢复**: 可以从事件日志完全恢复状态
4. **一致性**: 确定性计算保证多节点结果一致
5. **可验证性**: 可以验证计算结果的正确性

### 与其他组件的关系

#### 微服务架构

```
Matching Service (Event Sourcing)
    ↓ 事件
Event Store Service
    ↓ 事件流
Trading Service (订阅事件)
Account Service (订阅事件)
Position Service (订阅事件)
```

#### 生产组件集成

- **清算系统**: 基于事件触发的清算
- **资金费率**: 基于事件计算的费率
- **市场数据**: 基于事件生成的市场数据
- **通知系统**: 基于事件的通知
- **数据库**: 事件作为数据源

### 性能考虑

#### Event Sourcing开销

- **写入延迟**: +100-300ns（可接受）
- **存储空间**: +20-50%（完整历史）
- **查询性能**: 通过快照优化

#### 确定性计算开销

- **计算延迟**: 几乎无开销（相同复杂度）
- **内存开销**: 无额外开销
- **CPU开销**: 无额外开销

### 使用建议

#### 开发环境

```cpp
// 使用Event Sourcing版本
MatchingEngineEventSourcing engine(instrument_id);
engine.initialize("./event_store");
engine.set_deterministic_mode(true);
```

#### 生产环境

```cpp
// 确保启用Event Sourcing和确定性计算
MatchingEngineEventSourcing engine(instrument_id);
engine.initialize("/data/event_store");
engine.set_deterministic_mode(true);

// 定期创建快照以提高查询性能
// 定期压缩旧事件以节省空间
```

### 总结

**核心功能设计：Event Sourcing + Deterministic Calculation**

这是系统的**核心设计原则**，所有关键组件都围绕这个设计：
- ✅ 完整的事件历史记录
- ✅ 确定性、可重现的计算
- ✅ 可审计、可验证的交易流程
- ✅ 分布式一致性保证

这确保了系统的：
- **可靠性**: 可以从事件重建状态
- **可审计性**: 完整的事件历史
- **一致性**: 确定性计算结果
- **可调试性**: 可以重放事件定位问题

