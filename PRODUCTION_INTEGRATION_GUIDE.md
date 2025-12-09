# 生产环境集成指南

## 概述

本指南说明如何集成和使用所有生产功能组件。

## 快速开始

### 1. 编译生产服务

```bash
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make production_service -j4
```

### 2. 运行生产服务

```bash
./production_service
```

服务将在 `http://localhost:8080` 启动。

## 组件集成

### 1. 用户认证和授权

```cpp
#include "core/auth_manager.h"

AuthManager auth;

// 注册用户
std::string error_msg;
auth.registerUser("username", "email@example.com", "password", error_msg);

// 登录
std::string token;
auth.login("username", "password", token, error_msg);

// 验证Token
UserID user_id;
std::vector<std::string> roles;
if (auth.verifyToken(token, user_id, roles)) {
    // 用户已认证
}
```

### 2. 清算系统

```cpp
#include "core/liquidation_engine.h"

LiquidationEngine liquidation;
liquidation.setPositionManager(&position_manager);
liquidation.setAccountManager(&account_manager);

// 计算风险度
auto risk = liquidation.calculateRiskLevel(user_id, instrument_id, current_price);

// 执行清算
if (risk.is_liquidatable) {
    auto result = liquidation.liquidate(user_id, instrument_id, current_price);
}
```

### 3. 资金费率

```cpp
#include "core/funding_rate_manager.h"

FundingRateManager funding;
funding.setPositionManager(&position_manager);
funding.setAccountManager(&account_manager);

// 计算资金费率
double rate = funding.calculateFundingRate(instrument_id, premium_index, interest_rate);

// 结算
if (funding.shouldSettle(instrument_id)) {
    auto settlements = funding.settleFunding(instrument_id, mark_price);
}
```

### 4. 市场数据

```cpp
#include "core/market_data_service.h"

MarketDataService market_data;

// 订阅数据
market_data.subscribe(user_id, instrument_id, MarketDataService::SUBSCRIBE_DEPTH);

// 获取深度
std::vector<PriceLevel> bids, asks;
market_data.getDepth(instrument_id, 10, bids, asks);

// 获取K线
auto klines = market_data.getKLine(instrument_id, 300, start_time, end_time);
```

### 5. API网关

```cpp
#include "core/api_gateway.h"

APIGateway gateway;
gateway.setAuthManager(&auth_manager);

// 添加路由
APIGateway::RouteRule rule;
rule.pattern = "/api/v1/orders";
rule.service = "trading-service";
rule.require_auth = true;
rule.rate_limit = 100;
gateway.addRoute(rule);

// 处理请求
APIGateway::RequestContext context;
// ... 填充context ...
auto response = gateway.handleRequest(context);
```

### 6. 监控系统

```cpp
#include "core/monitoring_system.h"

MonitoringSystem monitoring;

// 记录指标
monitoring.recordOrderSubmitted(instrument_id);
monitoring.recordTrade(instrument_id, quantity);
monitoring.recordLatency("matching", latency_ms);

// 获取Prometheus格式指标
std::string metrics = monitoring.getPrometheusMetrics();

// 添加告警规则
MonitoringSystem::AlertRule rule;
rule.name = "high_latency";
rule.metric_name = "operation_latency_ms";
rule.condition = "value > 100";
rule.severity = "warning";
monitoring.addAlertRule(rule);
```

### 7. 通知服务

```cpp
#include "core/notification_service.h"

NotificationService notifications;

// 发送通知
notifications.notifyOrderFilled(user_id, order_id, instrument_id, quantity, price);
notifications.notifyLiquidation(user_id, instrument_id, quantity, price);

// 注册发送回调
notifications.setSendCallback(NotificationService::EMAIL, [](const Notification& n) {
    // 发送邮件
    return true;
});
```

### 8. 数据库

```cpp
#include "core/database_manager.h"

DatabaseManager db(DatabaseManager::SQLITE, "data/exchange.db");
db.connect();

// 插入订单
db.insertOrder(order);

// 插入交易
db.insertTrade(trade);

// 批量操作
db.batchInsertTrades(trades);
```

### 9. REST API服务器

```cpp
#include "core/rest_api_server.h"

RESTAPIServer server(8080);
server.setAuthManager(&auth_manager);

// 注册路由
server.registerRoute(RESTAPIServer::POST, "/api/v1/orders",
                    [](const RESTAPIServer::HTTPRequest& req, UserID user_id) {
                        // 处理订单提交
                        return RESTAPIServer::HTTPResponse{200, R"({"success": true})"};
                    }, true);

server.start();
```

## 完整示例

参考 `examples/basic_usage_example.cpp` 查看完整的使用示例。

编译示例：
```bash
cd build
make basic_usage_example
./basic_usage_example
```

## 配置

### 数据库配置

```cpp
// SQLite (开发/测试)
DatabaseManager db(DatabaseManager::SQLITE, "data/exchange.db");

// MySQL (生产)
DatabaseManager db(DatabaseManager::MYSQL, "host=localhost;db=exchange;user=root;pass=password");

// PostgreSQL (生产)
DatabaseManager db(DatabaseManager::POSTGRESQL, "postgresql://user:pass@localhost/exchange");
```

### 监控配置

监控系统暴露Prometheus格式指标，可通过 `/metrics` 端点访问。

集成Prometheus：
```yaml
scrape_configs:
  - job_name: 'exchange'
    static_configs:
      - targets: ['localhost:8080']
```

### 通知配置

配置通知发送回调：
```cpp
// 邮件通知
notification_service.setSendCallback(NotificationService::EMAIL, send_email);

// 短信通知
notification_service.setSendCallback(NotificationService::SMS, send_sms);

// 推送通知
notification_service.setSendCallback(NotificationService::PUSH, send_push);
```

## API端点

### 用户相关
- `POST /api/v1/users/register` - 注册
- `POST /api/v1/users/login` - 登录
- `GET /api/v1/users/profile` - 获取个人信息

### 订单相关
- `POST /api/v1/orders` - 提交订单
- `DELETE /api/v1/orders/{id}` - 取消订单
- `GET /api/v1/orders/{id}` - 查询订单
- `GET /api/v1/orders` - 查询订单列表

### 账户相关
- `GET /api/v1/account` - 查询账户
- `GET /api/v1/account/balance/history` - 余额历史

### 持仓相关
- `GET /api/v1/positions` - 查询持仓
- `GET /api/v1/positions/{instrument_id}/history` - 持仓历史

### 市场数据
- `GET /api/v1/market/depth` - 获取深度
- `GET /api/v1/market/trades` - 获取成交
- `GET /api/v1/market/kline` - 获取K线
- `GET /api/v1/market/ticker` - 24小时统计

### API密钥
- `POST /api/v1/api-keys` - 创建API密钥
- `GET /api/v1/api-keys` - 查询API密钥
- `DELETE /api/v1/api-keys/{id}` - 撤销API密钥

## 部署建议

### Docker部署

```dockerfile
FROM ubuntu:22.04
RUN apt-get update && apt-get install -y libssl-dev libcrypto++-dev
COPY build/production_service /app/
CMD ["/app/production_service"]
```

### Kubernetes部署

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: exchange-service
spec:
  replicas: 3
  template:
    spec:
      containers:
      - name: exchange
        image: exchange:latest
        ports:
        - containerPort: 8080
```

## 监控和告警

### 关键指标

- `orders_submitted_total` - 订单提交数
- `orders_filled_total` - 订单成交数
- `trades_volume_total` - 交易量
- `operation_latency_ms` - 操作延迟
- `system_cpu_usage_percent` - CPU使用率
- `system_memory_usage_bytes` - 内存使用

### 告警规则示例

```cpp
// 高延迟告警
AlertRule rule;
rule.name = "high_matching_latency";
rule.metric_name = "operation_latency_ms";
rule.condition = "value > 1000";  // > 1ms
rule.severity = "critical";
rule.duration_seconds = 60;  // 持续1分钟
monitoring.addAlertRule(rule);
```

## 安全建议

1. **密码**: 生产环境使用bcrypt/scrypt代替SHA256
2. **JWT**: 使用标准JWT库，定期轮换密钥
3. **API密钥**: 加密存储，支持IP白名单
4. **HTTPS**: 所有API通信使用TLS
5. **限流**: 配置适当的限流规则
6. **日志**: 记录所有敏感操作

## 性能优化

1. **数据库连接池**: 使用连接池管理数据库连接
2. **缓存**: 使用Redis缓存热点数据
3. **异步处理**: 通知和日志异步处理
4. **批量操作**: 使用批量插入提高性能
5. **索引**: 为常用查询创建索引

## 故障排查

### 常见问题

1. **数据库连接失败**: 检查连接字符串和权限
2. **认证失败**: 检查JWT密钥配置
3. **高延迟**: 检查数据库查询和网络延迟
4. **内存泄漏**: 使用Valgrind检查

### 日志位置

- 应用日志: `logs/production_service.log`
- 错误日志: `logs/error.log`
- 访问日志: `logs/access.log`

## 下一步

1. 配置生产数据库
2. 设置监控和告警
3. 配置通知服务
4. 性能测试和优化
5. 安全审计


## 概述

本指南说明如何集成和使用所有生产功能组件。

## 快速开始

### 1. 编译生产服务

```bash
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make production_service -j4
```

### 2. 运行生产服务

```bash
./production_service
```

服务将在 `http://localhost:8080` 启动。

## 组件集成

### 1. 用户认证和授权

```cpp
#include "core/auth_manager.h"

AuthManager auth;

// 注册用户
std::string error_msg;
auth.registerUser("username", "email@example.com", "password", error_msg);

// 登录
std::string token;
auth.login("username", "password", token, error_msg);

// 验证Token
UserID user_id;
std::vector<std::string> roles;
if (auth.verifyToken(token, user_id, roles)) {
    // 用户已认证
}
```

### 2. 清算系统

```cpp
#include "core/liquidation_engine.h"

LiquidationEngine liquidation;
liquidation.setPositionManager(&position_manager);
liquidation.setAccountManager(&account_manager);

// 计算风险度
auto risk = liquidation.calculateRiskLevel(user_id, instrument_id, current_price);

// 执行清算
if (risk.is_liquidatable) {
    auto result = liquidation.liquidate(user_id, instrument_id, current_price);
}
```

### 3. 资金费率

```cpp
#include "core/funding_rate_manager.h"

FundingRateManager funding;
funding.setPositionManager(&position_manager);
funding.setAccountManager(&account_manager);

// 计算资金费率
double rate = funding.calculateFundingRate(instrument_id, premium_index, interest_rate);

// 结算
if (funding.shouldSettle(instrument_id)) {
    auto settlements = funding.settleFunding(instrument_id, mark_price);
}
```

### 4. 市场数据

```cpp
#include "core/market_data_service.h"

MarketDataService market_data;

// 订阅数据
market_data.subscribe(user_id, instrument_id, MarketDataService::SUBSCRIBE_DEPTH);

// 获取深度
std::vector<PriceLevel> bids, asks;
market_data.getDepth(instrument_id, 10, bids, asks);

// 获取K线
auto klines = market_data.getKLine(instrument_id, 300, start_time, end_time);
```

### 5. API网关

```cpp
#include "core/api_gateway.h"

APIGateway gateway;
gateway.setAuthManager(&auth_manager);

// 添加路由
APIGateway::RouteRule rule;
rule.pattern = "/api/v1/orders";
rule.service = "trading-service";
rule.require_auth = true;
rule.rate_limit = 100;
gateway.addRoute(rule);

// 处理请求
APIGateway::RequestContext context;
// ... 填充context ...
auto response = gateway.handleRequest(context);
```

### 6. 监控系统

```cpp
#include "core/monitoring_system.h"

MonitoringSystem monitoring;

// 记录指标
monitoring.recordOrderSubmitted(instrument_id);
monitoring.recordTrade(instrument_id, quantity);
monitoring.recordLatency("matching", latency_ms);

// 获取Prometheus格式指标
std::string metrics = monitoring.getPrometheusMetrics();

// 添加告警规则
MonitoringSystem::AlertRule rule;
rule.name = "high_latency";
rule.metric_name = "operation_latency_ms";
rule.condition = "value > 100";
rule.severity = "warning";
monitoring.addAlertRule(rule);
```

### 7. 通知服务

```cpp
#include "core/notification_service.h"

NotificationService notifications;

// 发送通知
notifications.notifyOrderFilled(user_id, order_id, instrument_id, quantity, price);
notifications.notifyLiquidation(user_id, instrument_id, quantity, price);

// 注册发送回调
notifications.setSendCallback(NotificationService::EMAIL, [](const Notification& n) {
    // 发送邮件
    return true;
});
```

### 8. 数据库

```cpp
#include "core/database_manager.h"

DatabaseManager db(DatabaseManager::SQLITE, "data/exchange.db");
db.connect();

// 插入订单
db.insertOrder(order);

// 插入交易
db.insertTrade(trade);

// 批量操作
db.batchInsertTrades(trades);
```

### 9. REST API服务器

```cpp
#include "core/rest_api_server.h"

RESTAPIServer server(8080);
server.setAuthManager(&auth_manager);

// 注册路由
server.registerRoute(RESTAPIServer::POST, "/api/v1/orders",
                    [](const RESTAPIServer::HTTPRequest& req, UserID user_id) {
                        // 处理订单提交
                        return RESTAPIServer::HTTPResponse{200, R"({"success": true})"};
                    }, true);

server.start();
```

## 完整示例

参考 `examples/basic_usage_example.cpp` 查看完整的使用示例。

编译示例：
```bash
cd build
make basic_usage_example
./basic_usage_example
```

## 配置

### 数据库配置

```cpp
// SQLite (开发/测试)
DatabaseManager db(DatabaseManager::SQLITE, "data/exchange.db");

// MySQL (生产)
DatabaseManager db(DatabaseManager::MYSQL, "host=localhost;db=exchange;user=root;pass=password");

// PostgreSQL (生产)
DatabaseManager db(DatabaseManager::POSTGRESQL, "postgresql://user:pass@localhost/exchange");
```

### 监控配置

监控系统暴露Prometheus格式指标，可通过 `/metrics` 端点访问。

集成Prometheus：
```yaml
scrape_configs:
  - job_name: 'exchange'
    static_configs:
      - targets: ['localhost:8080']
```

### 通知配置

配置通知发送回调：
```cpp
// 邮件通知
notification_service.setSendCallback(NotificationService::EMAIL, send_email);

// 短信通知
notification_service.setSendCallback(NotificationService::SMS, send_sms);

// 推送通知
notification_service.setSendCallback(NotificationService::PUSH, send_push);
```

## API端点

### 用户相关
- `POST /api/v1/users/register` - 注册
- `POST /api/v1/users/login` - 登录
- `GET /api/v1/users/profile` - 获取个人信息

### 订单相关
- `POST /api/v1/orders` - 提交订单
- `DELETE /api/v1/orders/{id}` - 取消订单
- `GET /api/v1/orders/{id}` - 查询订单
- `GET /api/v1/orders` - 查询订单列表

### 账户相关
- `GET /api/v1/account` - 查询账户
- `GET /api/v1/account/balance/history` - 余额历史

### 持仓相关
- `GET /api/v1/positions` - 查询持仓
- `GET /api/v1/positions/{instrument_id}/history` - 持仓历史

### 市场数据
- `GET /api/v1/market/depth` - 获取深度
- `GET /api/v1/market/trades` - 获取成交
- `GET /api/v1/market/kline` - 获取K线
- `GET /api/v1/market/ticker` - 24小时统计

### API密钥
- `POST /api/v1/api-keys` - 创建API密钥
- `GET /api/v1/api-keys` - 查询API密钥
- `DELETE /api/v1/api-keys/{id}` - 撤销API密钥

## 部署建议

### Docker部署

```dockerfile
FROM ubuntu:22.04
RUN apt-get update && apt-get install -y libssl-dev libcrypto++-dev
COPY build/production_service /app/
CMD ["/app/production_service"]
```

### Kubernetes部署

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: exchange-service
spec:
  replicas: 3
  template:
    spec:
      containers:
      - name: exchange
        image: exchange:latest
        ports:
        - containerPort: 8080
```

## 监控和告警

### 关键指标

- `orders_submitted_total` - 订单提交数
- `orders_filled_total` - 订单成交数
- `trades_volume_total` - 交易量
- `operation_latency_ms` - 操作延迟
- `system_cpu_usage_percent` - CPU使用率
- `system_memory_usage_bytes` - 内存使用

### 告警规则示例

```cpp
// 高延迟告警
AlertRule rule;
rule.name = "high_matching_latency";
rule.metric_name = "operation_latency_ms";
rule.condition = "value > 1000";  // > 1ms
rule.severity = "critical";
rule.duration_seconds = 60;  // 持续1分钟
monitoring.addAlertRule(rule);
```

## 安全建议

1. **密码**: 生产环境使用bcrypt/scrypt代替SHA256
2. **JWT**: 使用标准JWT库，定期轮换密钥
3. **API密钥**: 加密存储，支持IP白名单
4. **HTTPS**: 所有API通信使用TLS
5. **限流**: 配置适当的限流规则
6. **日志**: 记录所有敏感操作

## 性能优化

1. **数据库连接池**: 使用连接池管理数据库连接
2. **缓存**: 使用Redis缓存热点数据
3. **异步处理**: 通知和日志异步处理
4. **批量操作**: 使用批量插入提高性能
5. **索引**: 为常用查询创建索引

## 故障排查

### 常见问题

1. **数据库连接失败**: 检查连接字符串和权限
2. **认证失败**: 检查JWT密钥配置
3. **高延迟**: 检查数据库查询和网络延迟
4. **内存泄漏**: 使用Valgrind检查

### 日志位置

- 应用日志: `logs/production_service.log`
- 错误日志: `logs/error.log`
- 访问日志: `logs/access.log`

## 下一步

1. 配置生产数据库
2. 设置监控和告警
3. 配置通知服务
4. 性能测试和优化
5. 安全审计

