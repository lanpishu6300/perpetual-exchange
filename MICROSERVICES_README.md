# 微服务架构实现

## 概述

将永续合约交易所拆分为两个核心微服务，保持**高内聚、低耦合**的设计原则。

## 服务架构

### 1. Matching Service (撮合服务)

**职责**: 核心撮合逻辑
- 订单撮合
- 订单簿管理
- 交易执行
- 价格-时间优先级

**特点**:
- **高内聚**: 所有撮合相关逻辑集中
- **低耦合**: 通过gRPC接口通信
- 超低延迟（纳秒级）
- 无状态（可水平扩展）

**接口** (gRPC):
- `ProcessOrder(OrderRequest) -> MatchResult`
- `CancelOrder(CancelOrderRequest) -> CancelOrderResponse`
- `GetOrderBook(InstrumentRequest) -> OrderBookSnapshot`
- `GetBestPrice(InstrumentRequest) -> BestPriceResponse`

### 2. Trading Service (交易服务)

**职责**: 交易业务逻辑
- 订单管理
- 账户管理
- 持仓管理
- 订单验证
- 风控检查

**特点**:
- **高内聚**: 订单、账户、持仓管理集中
- **低耦合**: 通过gRPC调用Matching Service
- 高吞吐量
- 数据持久化

**接口** (gRPC):
- `SubmitOrder(SubmitOrderRequest) -> SubmitOrderResponse`
- `CancelOrder(CancelOrderRequest) -> CancelOrderResponse`
- `QueryOrder(QueryOrderRequest) -> QueryOrderResponse`
- `QueryUserOrders(QueryUserOrdersRequest) -> QueryUserOrdersResponse`
- `QueryAccount(QueryAccountRequest) -> QueryAccountResponse`
- `QueryPosition(QueryPositionRequest) -> QueryPositionResponse`

## 架构设计

### 高内聚

**Matching Service**:
- ✅ 撮合算法集中
- ✅ 订单簿管理集中
- ✅ 交易执行集中
- ✅ 价格优先级逻辑集中

**Trading Service**:
- ✅ 订单生命周期管理集中
- ✅ 账户管理集中
- ✅ 持仓管理集中
- ✅ 业务规则集中

### 低耦合

- ✅ 通过gRPC接口通信
- ✅ 接口版本化
- ✅ 独立部署
- ✅ 独立扩展
- ✅ 数据隔离

## 通信流程

```
客户端
  │
  │ SubmitOrder
  ▼
Trading Service
  │ 1. 验证订单
  │ 2. 检查账户
  │ 3. 检查持仓限制
  │ 4. 冻结保证金
  │
  │ ProcessOrder
  ▼
Matching Service
  │ 1. 撮合订单
  │ 2. 更新订单簿
  │ 3. 生成交易
  │
  │ MatchResult
  ▼
Trading Service
  │ 1. 更新订单状态
  │ 2. 更新账户余额
  │ 3. 更新持仓
  │ 4. 返回结果
  │
  │ SubmitOrderResponse
  ▼
客户端
```

## 文件结构

```
services/
├── proto/
│   ├── matching.proto          # Matching Service接口定义
│   └── trading.proto           # Trading Service接口定义
│
├── matching_service/
│   ├── matching_service.h      # Matching Service实现
│   ├── matching_service.cpp
│   └── main.cpp                # 服务入口
│
└── trading_service/
    ├── trading_service.h       # Trading Service实现
    ├── trading_service.cpp
    └── main.cpp                # 服务入口
```

## 快速开始

### 本地运行

```bash
# 1. 编译（需要gRPC）
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make matching_service trading_service -j4

# 2. 启动撮合服务（终端1）
./matching_service 1 0.0.0.0:50051 ./event_store

# 3. 启动交易服务（终端2）
./trading_service 0.0.0.0:50052 localhost:50051
```

### Docker运行

```bash
# 构建并启动
docker compose -f docker-compose.microservices.yml up --build
```

## 性能特点

- **Matching Service**: <100ns 延迟（本地调用）
- **Trading Service**: <1ms 延迟（包含网络）
- **整体延迟**: <2ms (端到端)

## 扩展性

### 水平扩展

**Matching Service**:
- 按合约拆分（每个合约一个实例）
- 无状态设计，易于扩展

**Trading Service**:
- 按用户ID分片
- 无状态水平扩展

### 垂直扩展

- CPU优化
- 内存优化
- NUMA绑定

## 详细文档

- [架构设计](./MICROSERVICES_ARCHITECTURE.md)
- [快速开始](./MICROSERVICES_QUICK_START.md)

