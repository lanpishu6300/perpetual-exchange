# 微服务架构设计文档

## 架构概述

将永续合约交易所拆分为两个核心微服务，保持**高内聚、低耦合**的原则。

## 服务拆分

### 1. Matching Service (撮合服务)
**职责**: 核心撮合逻辑
- 订单撮合
- 订单簿管理
- 交易执行
- 价格-时间优先级

**特点**:
- **高内聚**: 所有撮合相关逻辑集中在这里
- **低耦合**: 通过gRPC接口与外部通信
- 超低延迟（纳秒级）
- 单线程或NUMA优化
- 内存数据库
- 无状态（可水平扩展）

**接口**:
- `ProcessOrder`: 处理订单并撮合
- `CancelOrder`: 取消订单
- `GetOrderBook`: 获取订单簿快照
- `GetBestPrice`: 获取最佳买卖价

**依赖**: Event Store Service（发布事件）

### 2. Trading Service (交易服务)
**职责**: 交易相关业务逻辑
- 订单管理（创建、查询、取消）
- 账户管理（余额、保证金）
- 持仓管理
- 订单验证
- 风控检查

**特点**:
- **高内聚**: 订单、账户、持仓管理集中在这里
- **低耦合**: 通过gRPC调用Matching Service
- 高吞吐量
- 数据持久化
- 业务逻辑处理

**接口**:
- `SubmitOrder`: 提交订单
- `CancelOrder`: 取消订单
- `QueryOrder`: 查询订单
- `QueryUserOrders`: 查询用户订单
- `QueryAccount`: 查询账户
- `QueryPosition`: 查询持仓

**依赖**: Matching Service（撮合服务）

## 服务通信

### gRPC 同步调用
- Trading Service → Matching Service
- 用于订单处理和撮合
- 低延迟、高性能

### 事件流（可选）
- Matching Service → Event Store Service
- 用于事件溯源和审计
- 异步、解耦

## 架构图

```
┌─────────────────────────────────────┐
│        Trading Service              │
│  (订单、账户、持仓管理)              │
│  - 订单验证                          │
│  - 账户管理                          │
│  - 持仓管理                          │
│  - 风控检查                          │
└──────────────┬──────────────────────┘
               │ gRPC
               │ (ProcessOrder, CancelOrder)
               ▼
┌─────────────────────────────────────┐
│        Matching Service             │
│  (核心撮合逻辑)                      │
│  - 订单撮合                          │
│  - 订单簿管理                        │
│  - 交易执行                          │
└──────────────┬──────────────────────┘
               │ Events
               ▼
┌─────────────────────────────────────┐
│      Event Store Service            │
│  (事件存储 - 可选)                   │
│  - 事件存储                          │
│  - 事件重放                          │
└─────────────────────────────────────┘
```

## 高内聚设计

### Matching Service 内聚性
- ✅ 撮合算法集中
- ✅ 订单簿管理集中
- ✅ 交易执行集中
- ✅ 价格优先级逻辑集中

### Trading Service 内聚性
- ✅ 订单生命周期管理集中
- ✅ 账户管理集中
- ✅ 持仓管理集中
- ✅ 业务规则集中

## 低耦合设计

### 服务间通信
- ✅ 通过定义良好的gRPC接口
- ✅ 接口版本化
- ✅ 错误处理标准化

### 数据隔离
- ✅ 每个服务独立数据存储
- ✅ 通过接口访问数据
- ✅ 避免直接数据库共享

### 部署独立
- ✅ 服务可独立部署
- ✅ 服务可独立扩展
- ✅ 服务可独立升级

## 技术栈

- **通信协议**: gRPC (Protocol Buffers)
- **序列化**: Protocol Buffers
- **服务发现**: Docker网络 / Kubernetes DNS
- **负载均衡**: gRPC内置 / Nginx
- **监控**: Prometheus + Grafana
- **日志**: 集中式日志收集
- **容器**: Docker + Docker Compose / Kubernetes

## 性能目标

- **Matching Service**: <100ns 延迟
- **Trading Service**: <1ms 延迟（包含网络）
- **整体延迟**: <2ms (端到端)

## 部署方式

### Docker Compose (开发/测试)
```bash
docker compose -f docker-compose.microservices.yml up
```

### Kubernetes (生产)
- Matching Service: StatefulSet (可扩展)
- Trading Service: Deployment (可扩展)
- Service Discovery: Kubernetes Service
- Load Balancing: Kubernetes Service LoadBalancer

## 扩展性

### 水平扩展
- **Matching Service**: 
  - 按合约拆分（每个合约一个实例）
  - 按分区拆分（订单簿分区）
- **Trading Service**: 
  - 按用户ID分片
  - 无状态水平扩展

### 垂直扩展
- **Matching Service**: 
  - CPU优化（NUMA绑定）
  - 内存优化（大内存页）
- **Trading Service**: 
  - 数据库连接池优化
  - 缓存优化

## 数据一致性

### 最终一致性
- 订单状态通过事件同步
- 账户余额最终一致
- 持仓信息最终一致

### 强一致性
- 撮合结果强一致
- 订单提交强一致
- 账户余额更新强一致（通过事务）

## 故障处理

### 服务降级
- Matching Service不可用: 订单排队
- Trading Service不可用: 只读模式

### 容错机制
- 重试机制
- 超时处理
- 熔断器模式

## 监控和日志

### 监控指标
- 服务可用性
- 请求延迟
- 错误率
- 吞吐量

### 日志
- 结构化日志
- 集中式日志收集
- 日志级别管理

## 安全性

### 认证授权
- gRPC认证（TLS）
- API密钥验证
- 用户权限管理

### 数据安全
- 传输加密（TLS）
- 敏感数据加密
- 审计日志

