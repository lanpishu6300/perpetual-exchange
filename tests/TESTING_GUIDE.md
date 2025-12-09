# 生产版本测试指南

## 测试分类

### 1. 单元测试 (Unit Tests)

测试单个组件的功能。

**位置**: `tests/unit/`

**测试文件**:
- `test_auth_manager.cpp` - 认证授权测试
- `test_liquidation_engine.cpp` - 清算系统测试
- `test_funding_rate_manager.cpp` - 资金费率测试
- `test_matching_engine_event_sourcing.cpp` - Event Sourcing撮合引擎测试

**运行**:
```bash
cd build
make test_auth_manager
./test_auth_manager
```

### 2. 集成测试 (Integration Tests)

测试多个组件的协作。

**位置**: `tests/integration/`

**测试文件**:
- `test_trading_workflow.cpp` - 完整交易流程测试

**运行**:
```bash
cd build
make test_trading_workflow
./test_trading_workflow
```

### 3. 性能测试 (Performance Tests)

测试性能指标。

**位置**: `tests/performance/`

**测试文件**:
- `test_production_performance.cpp` - 生产性能测试

**运行**:
```bash
cd build
make test_production_performance
./test_production_performance
```

### 4. 压力测试 (Stress Tests)

测试系统极限。

**位置**: `tests/production/`

**测试文件**:
- `load_test.cpp` - 负载测试
- `stress_test.cpp` - 压力测试
- `functional_test.cpp` - 功能测试

**运行**:
```bash
cd build
make production_load_test
./production_load_test 60 1000  # 60秒，1000 orders/sec

make production_stress_test
./production_stress_test 8 10000  # 8线程，每线程10000订单

make production_functional_test
./production_functional_test
```

## 测试覆盖

### 功能覆盖

- [x] 用户认证和授权
- [x] 订单处理
- [x] 订单撮合
- [x] 账户管理
- [x] Event Sourcing
- [x] 清算系统
- [x] 资金费率
- [x] 市场数据
- [x] 监控系统
- [x] 通知系统

### 性能指标

**目标性能**:
- 撮合延迟: P99 < 100μs
- 查询延迟: P99 < 10μs
- 吞吐量: > 100K orders/sec
- Event Sourcing开销: < 500ns

## 运行所有测试

### 使用CMake

```bash
cd build
cmake ..
make
ctest
```

### 手动运行

```bash
# 单元测试
./test_auth_manager
./test_liquidation_engine
./test_funding_rate_manager
./test_matching_engine_event_sourcing

# 集成测试
./test_trading_workflow

# 性能测试
./test_production_performance

# 生产测试
./production_functional_test
./production_load_test 60 1000
./production_stress_test 8 10000
```

## 测试报告

测试结果会输出到控制台，包括：
- 测试通过/失败状态
- 性能指标（延迟、吞吐量）
- 错误信息

## 持续集成

建议在CI/CD中运行：
1. 单元测试（快速反馈）
2. 集成测试（验证组件协作）
3. 性能测试（性能回归检测）

## 注意事项

1. 测试数据存储在临时目录，测试后自动清理
2. 性能测试需要足够的CPU和内存
3. 压力测试可能产生大量事件数据

