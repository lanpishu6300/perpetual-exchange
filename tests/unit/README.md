# 单元测试目录

本目录包含所有核心组件的单元测试。

## 测试文件列表

| 测试文件 | 测试组件 | 测试用例数 |
|---------|---------|-----------|
| `test_account_manager.cpp` | AccountBalanceManager | ~15 |
| `test_position_manager.cpp` | PositionManager | ~12 |
| `test_orderbook.cpp` | OrderBook | ~12 |
| `test_order_validator.cpp` | OrderValidator | ~12 |
| `test_auth_manager.cpp` | AuthManager | ~5 |
| `test_liquidation_engine.cpp` | LiquidationEngine | ~8 |
| `test_funding_rate_manager.cpp` | FundingRateManager | ~8 |
| `test_matching_engine_event_sourcing.cpp` | MatchingEngineEventSourcing | ~8 |

## 快速开始

### 运行所有单元测试

```bash
# 从项目根目录
./run_unit_tests.sh
```

### 运行单个测试

```bash
cd build
./test_account_manager
```

### 运行特定测试用例

```bash
cd build
./test_account_manager --gtest_filter=AccountManagerTest.BasicBalanceOperations
```

## 测试覆盖详情

### AccountManager 测试

- **基本操作**: 余额设置、更新、查询
- **冻结机制**: 冻结、解冻、可用余额计算
- **保证金**: 保证金计算、充足性检查
- **多用户**: 多用户隔离、并发安全
- **边界情况**: 零余额、负值处理、溢出保护

### PositionManager 测试

- **持仓操作**: 买入、卖出、持仓更新
- **多空持仓**: 多头、空头、持仓转换
- **持仓限制**: 限制检查、限制更新
- **多品种**: 多品种隔离、独立管理
- **持仓计算**: 新持仓大小计算、平仓

### OrderBook 测试

- **订单管理**: 插入、删除、查找
- **价格层级**: 最佳买卖价、价差计算
- **匹配检查**: 订单匹配条件检查
- **排序**: 价格时间优先级排序
- **边界情况**: 空订单簿、无效订单

### OrderValidator 测试

- **基本验证**: 订单ID、用户ID、价格、数量
- **范围验证**: 价格范围、数量范围
- **精度验证**: 价格精度、数量步长
- **组合验证**: 多规则组合验证

### LiquidationEngine 测试

- **风险计算**: 风险等级、风险比率
- **清算触发**: 清算条件检查
- **保证金**: 维持保证金计算
- **场景测试**: 低保证金、无持仓场景

### FundingRateManager 测试

- **费率计算**: 资金费率计算、边界检查
- **溢价指数**: 溢价指数更新、计算
- **结算管理**: 结算时间设置、检查
- **多品种**: 多品种独立管理

### MatchingEngineEventSourcing 测试

- **订单处理**: 订单提交、处理
- **事件溯源**: 事件存储、回放
- **撮合**: 订单撮合、成交生成
- **确定性**: 确定性模式验证

## 测试统计

- **总测试文件**: 8
- **总测试用例**: ~80+
- **测试覆盖**: 核心功能、边界情况、错误处理、并发安全

## 维护指南

### 添加新测试

1. 在对应的测试文件中添加新的 `TEST_F` 用例
2. 遵循 AAA 模式（Arrange-Act-Assert）
3. 使用清晰的测试名称
4. 确保测试独立且可重复

### 更新测试

1. 当组件接口变化时，更新相应测试
2. 保持测试与实现同步
3. 删除过时的测试用例

### 测试最佳实践

- ✅ 每个测试只测试一个功能点
- ✅ 使用有意义的测试数据
- ✅ 测试正常路径和错误路径
- ✅ 测试边界情况
- ✅ 保持测试快速运行
- ✅ 使用描述性的测试名称

## 相关文档

- [测试指南](../TESTING_GUIDE.md)
- [项目架构](../../ARCHITECTURE.md)

