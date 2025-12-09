# Event Sourcing + Deterministic Calculation 核心设计

## 概述

**是的，核心功能依然是 Event Sourcing + Deterministic Calculation 的设计。**

这是系统的**核心设计原则**，所有关键交易逻辑都基于这个设计。

## 核心组件

### 1. MatchingEngineEventSourcing

**核心撮合引擎**，集成Event Sourcing和Deterministic Calculation：

```cpp
class MatchingEngineEventSourcing : public MatchingEngine {
    // Event Sourcing
    EventStore* event_store_;
    EventPublisher* event_publisher_;
    
    // Deterministic Calculation
    bool deterministic_mode_;
    
    // 使用确定性计算的撮合
    std::vector<Trade> match_order_deterministic(Order* order);
};
```

**特点**:
- ✅ 所有订单操作都记录为事件
- ✅ 所有计算使用确定性算法
- ✅ 可以从事件完全重建状态
- ✅ 计算结果可重现

### 2. EventStore

**事件存储**，追加式日志：

- 所有状态变更都作为事件存储
- 支持事件查询和重放
- 支持快照和压缩

### 3. DeterministicCalculator

**确定性计算器**：

- 确定性价格比较
- 确定性撮合价格计算
- 确定性时间戳生成
- 固定点运算（避免浮点误差）

## 工作流程

### 订单处理流程（Event Sourcing）

```
1. 接收订单
   ↓
2. 发出事件: OrderPlacedEvent
   ↓
3. 确定性撮合（DeterministicCalculator）
   ↓
4. 如果成交，发出事件: TradeExecutedEvent
   ↓
5. 更新订单，发出事件: OrderUpdatedEvent
   ↓
6. 所有事件存储到EventStore
   ↓
7. 事件异步发布到订阅者
```

### 确定性计算示例

```cpp
// 价格比较（确定性）
bool can_match = DeterministicCalculator::comparePrice(
    buy_order->price, 
    sell_order->price
);

// 撮合价格（确定性）
Price match_price = DeterministicCalculator::calculateMatchPrice(
    buy_order->price,
    sell_order->price,
    buy_order->sequence_id,  // 使用序列ID，不依赖系统时间
    sell_order->sequence_id
);
```

## 在生产服务中的使用

### 生产服务配置

```cpp
// src/production_service_main.cpp

// 使用Event Sourcing版本的撮合引擎
MatchingEngineEventSourcing matching_engine(1);
matching_engine.initialize("data/event_store");
matching_engine.set_deterministic_mode(true);  // 启用确定性计算
```

### 微服务中的使用

```cpp
// services/matching_service/matching_service.cpp

// Matching Service使用Event Sourcing版本
MatchingEngineEventSourcing matching_engine_(instrument_id);
matching_engine_->initialize(event_store_dir);
matching_engine_->set_deterministic_mode(true);
```

## 事件类型

系统记录的所有事件：

1. **OrderPlacedEvent** - 订单提交
2. **OrderCancelledEvent** - 订单取消
3. **OrderRejectedEvent** - 订单拒绝
4. **TradeExecutedEvent** - 交易执行
5. **PositionOpenedEvent** - 开仓
6. **PositionClosedEvent** - 平仓
7. **FundingSettledEvent** - 资金费率结算
8. **LiquidationEvent** - 强制平仓

## 状态重建

可以从事件完全重建系统状态：

```cpp
// 重放所有事件重建状态
matching_engine.replay_events(0, UINT64_MAX);

// 重放特定范围的事件
matching_engine.replay_events(start_sequence, end_sequence);
```

## 优势

### 对交易系统的重要性

1. **合规性**: 完整的事件历史满足监管要求
2. **审计**: 可以追踪每一笔交易的完整历史
3. **故障恢复**: 可以从事件日志完全恢复状态
4. **一致性**: 确定性计算保证多节点结果一致
5. **可验证性**: 可以验证计算结果的正确性
6. **可调试性**: 可以重放事件定位问题

## 与其他组件的关系

### 清算系统

清算系统基于Event Sourcing的事件触发：

```cpp
// 清算系统订阅清算事件
event_publisher->subscribe(EventType::LIQUIDATION, [&](const Event& e) {
    // 处理清算事件
});
```

### 资金费率

资金费率基于事件计算：

```cpp
// 资金费率结算时发出事件
FundingSettledEvent event;
event.user_id = user_id;
event.payment = payment;
event_store->append(event);
```

### 市场数据

市场数据从事件生成：

```cpp
// 交易事件触发市场数据更新
event_publisher->subscribe(EventType::TRADE_EXECUTED, [&](const Event& e) {
    market_data_service.updateTrade(e.trade);
});
```

## 性能

### Event Sourcing开销

- **写入延迟**: +100-300ns（可接受）
- **存储空间**: +20-50%（完整历史）
- **查询性能**: 通过快照优化，不影响热路径

### 确定性计算开销

- **计算延迟**: 几乎无开销
- **内存开销**: 无额外开销
- **CPU开销**: 无额外开销

## 配置

### 启用Event Sourcing和确定性计算

```cpp
// 初始化
MatchingEngineEventSourcing engine(instrument_id);
engine.initialize("data/event_store");

// 启用确定性模式
engine.set_deterministic_mode(true);
```

### 生产环境配置

```cpp
// 1. 初始化Event Store
EventStore event_store;
event_store.initialize("/data/event_store");

// 2. 创建撮合引擎
MatchingEngineEventSourcing engine(instrument_id, &event_store);
engine.set_deterministic_mode(true);

// 3. 启用高级功能
// - 事件压缩
// - 快照创建
// - 分布式复制
```

## 总结

**核心功能设计：Event Sourcing + Deterministic Calculation**

- ✅ 所有订单处理都记录为事件
- ✅ 所有计算都使用确定性算法
- ✅ 可以从事件完全重建状态
- ✅ 计算结果完全可重现
- ✅ 完整的事件历史记录
- ✅ 满足合规和审计要求

这是系统的**核心设计原则**，确保系统的可靠性、一致性和可审计性。

