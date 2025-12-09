# 微服务快速开始指南

## 架构说明

系统拆分为两个微服务：

1. **Matching Service (撮合服务)**: 核心撮合逻辑
2. **Trading Service (交易服务)**: 订单、账户、持仓管理

## 本地开发

### 1. 编译（需要gRPC支持）

```bash
# 安装gRPC（Ubuntu/Debian）
sudo apt-get install -y libgrpc++-dev protobuf-compiler libprotobuf-dev

# 编译
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make matching_service trading_service -j4
```

### 2. 运行服务

**终端1 - 启动撮合服务:**
```bash
cd build
./matching_service 1 0.0.0.0:50051 ./event_store
```

**终端2 - 启动交易服务:**
```bash
cd build
./trading_service 0.0.0.0:50052 localhost:50051
```

## Docker 部署

### 快速启动

```bash
docker compose -f docker-compose.microservices.yml up
```

### 服务地址

- **Matching Service**: `localhost:50051`
- **Trading Service**: `localhost:50052`

## 服务接口

### Matching Service (gRPC)

**端口**: 50051

**接口**:
- `ProcessOrder`: 处理订单并撮合
- `CancelOrder`: 取消订单
- `GetOrderBook`: 获取订单簿
- `GetBestPrice`: 获取最佳价格

### Trading Service (gRPC)

**端口**: 50052

**接口**:
- `SubmitOrder`: 提交订单
- `CancelOrder`: 取消订单
- `QueryOrder`: 查询订单
- `QueryUserOrders`: 查询用户订单
- `QueryAccount`: 查询账户
- `QueryPosition`: 查询持仓

## 测试

### 使用gRPC客户端测试

```bash
# 安装 grpcurl
go install github.com/fullstorydev/grpcurl/cmd/grpcurl@latest

# 列出服务
grpcurl -plaintext localhost:50052 list

# 调用接口
grpcurl -plaintext -d '{"user_id": 1001, "instrument_id": 1, "side": "BUY", "order_type": "LIMIT", "price": 50000000000, "quantity": 1000000}' \
  localhost:50052 perpetual.trading.TradingService/SubmitOrder
```

## 架构优势

### 高内聚
- ✅ Matching Service: 所有撮合逻辑集中
- ✅ Trading Service: 所有交易业务逻辑集中

### 低耦合
- ✅ 通过gRPC接口通信
- ✅ 独立部署和扩展
- ✅ 服务间无直接依赖

### 可扩展性
- ✅ Matching Service: 可按合约水平扩展
- ✅ Trading Service: 可按用户分片扩展

### 性能
- ✅ Matching Service: 超低延迟（纳秒级）
- ✅ Trading Service: 高吞吐量

## 监控

### 健康检查

```bash
# Matching Service
grpcurl -plaintext localhost:50051 list

# Trading Service
grpcurl -plaintext localhost:50052 list
```

### 日志

日志文件位置：
- Matching Service: `./logs/matching_service.log`
- Trading Service: `./logs/trading_service.log`

## 故障排查

### 服务无法启动

1. 检查端口是否被占用
2. 检查gRPC库是否安装
3. 检查配置文件

### 服务间通信失败

1. 检查网络连接
2. 检查服务地址配置
3. 检查防火墙设置

## 下一步

- 添加API Gateway
- 添加服务发现（Consul/etcd）
- 添加监控（Prometheus）
- 添加日志收集（ELK）
- 添加链路追踪（Jaeger）

