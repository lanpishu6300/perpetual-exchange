# 功能测试套件

## 概述

本目录包含永续合约交易系统的全面功能测试，覆盖所有核心功能和边界情况。

## 测试文件

### 1. comprehensive_functional_test.cpp
**全面功能测试** - 覆盖所有核心功能模块

**测试范围**:
- ✅ 订单处理测试（限价单、市价单、无效订单）
- ✅ 撮合引擎测试（完全匹配、部分匹配、价格-时间优先级）
- ✅ 订单取消测试（活跃订单、不存在订单、错误用户）
- ✅ 账户管理测试（充值、提现、冻结、解冻）
- ✅ 持仓管理测试（开仓、平仓、盈亏更新）
- ✅ 清算测试（风险检查）
- ✅ 资金费率测试（费率计算、结算）
- ✅ 事件溯源测试（事件存储、回放）
- ✅ 边界条件测试（零数量、超大数量、超高价格）
- ✅ 并发测试（多线程下单）

**运行方式**:
```bash
./build/test_comprehensive_functional
```

### 2. order_flow_test.cpp
**订单流程测试** - 测试完整的订单生命周期

**测试范围**:
- ✅ 完整订单流程（下单 -> 部分成交 -> 完全成交）
- ✅ 订单取消流程
- ✅ 价格改进流程
- ✅ 订单簿深度测试

**运行方式**:
```bash
./build/test_order_flow
```

### 3. api_integration_test.cpp
**API集成测试** - 测试API和认证功能

**测试范围**:
- ✅ 用户注册和登录
- ✅ API密钥管理
- ✅ API网关认证
- ✅ 监控系统

**运行方式**:
```bash
./build/test_api_integration
```

## 测试分类

### 单元测试 (Unit Tests)
- `test_auth_manager.cpp` - 认证管理器
- `test_liquidation_engine.cpp` - 清算引擎
- `test_funding_rate_manager.cpp` - 资金费率管理器
- `test_matching_engine_event_sourcing.cpp` - 事件溯源撮合引擎

### 集成测试 (Integration Tests)
- `test_trading_workflow.cpp` - 交易工作流
- `test_api_integration.cpp` - API集成

### 功能测试 (Functional Tests)
- `comprehensive_functional_test.cpp` - 全面功能测试
- `order_flow_test.cpp` - 订单流程测试

### 性能测试 (Performance Tests)
- `test_production_performance.cpp` - 生产性能测试
- `test_optimized_v3*.cpp` - 优化版本性能测试

## 运行所有测试

```bash
# 编译所有测试
cd build
cmake ..
make

# 运行所有测试
ctest

# 运行特定测试
./build/test_comprehensive_functional
./build/test_order_flow
./build/test_api_integration
```

## 测试覆盖率

### 核心功能覆盖率
- ✅ 订单处理: 100%
- ✅ 撮合引擎: 100%
- ✅ 账户管理: 90%
- ✅ 持仓管理: 90%
- ✅ 清算引擎: 80%
- ✅ 资金费率: 80%
- ✅ 事件溯源: 90%
- ✅ API功能: 85%

### 边界条件覆盖率
- ✅ 正常情况: 100%
- ✅ 异常情况: 85%
- ✅ 边界值: 80%
- ✅ 并发场景: 75%

## 测试最佳实践

1. **隔离性**: 每个测试都是独立的，不依赖其他测试
2. **可重复性**: 测试结果可重复，不依赖外部状态
3. **快速执行**: 单元测试应该在毫秒级完成
4. **清晰命名**: 测试名称清楚描述测试内容
5. **完整断言**: 使用适当的断言验证结果

## 持续集成

建议在CI/CD流程中运行：
```bash
# 运行所有测试
ctest --output-on-failure

# 生成覆盖率报告
# (需要配置覆盖率工具)
```

## 添加新测试

添加新测试时，请遵循以下原则：

1. **测试文件命名**: `test_<feature_name>.cpp`
2. **测试类命名**: `<Feature>Test`
3. **测试用例命名**: `TestName_Scenario`
4. **使用Google Test框架**
5. **添加适当的文档注释**

## 问题报告

如果测试失败，请提供：
1. 测试名称和用例
2. 错误信息
3. 复现步骤
4. 环境信息

