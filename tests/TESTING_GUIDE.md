# 单元测试指南

本文档介绍如何运行和维护项目的单元测试。

## 目录

- [概述](#概述)
- [测试结构](#测试结构)
- [运行测试](#运行测试)
- [测试覆盖](#测试覆盖)
- [编写新测试](#编写新测试)
- [最佳实践](#最佳实践)

## 概述

项目使用 Google Test (gtest) 框架进行单元测试。所有单元测试位于 `tests/unit/` 目录下。

### 测试框架

- **框架**: Google Test (gtest)
- **构建系统**: CMake
- **测试语言**: C++

## 测试结构

```
tests/
├── unit/                          # 单元测试
│   ├── test_account_manager.cpp   # 账户管理器测试
│   ├── test_position_manager.cpp  # 持仓管理器测试
│   ├── test_orderbook.cpp         # 订单簿测试
│   ├── test_order_validator.cpp   # 订单验证器测试
│   ├── test_auth_manager.cpp      # 认证管理器测试
│   ├── test_liquidation_engine.cpp # 清算引擎测试
│   ├── test_funding_rate_manager.cpp # 资金费率管理器测试
│   └── test_matching_engine_event_sourcing.cpp # 撮合引擎测试
├── integration/                   # 集成测试
├── functional/                   # 功能测试
└── performance/                  # 性能测试
```

## 运行测试

### 方法1: 使用测试脚本（推荐）

```bash
# 运行所有单元测试
./run_unit_tests.sh
```

脚本会自动：
- 检查并创建构建目录
- 编译所有单元测试
- 运行所有测试
- 生成测试报告
- 显示测试摘要

### 方法2: 使用 CMake/CTest

```bash
# 进入构建目录
cd build

# 配置（如果还没有）
cmake ..

# 编译所有测试
make

# 运行所有测试
ctest

# 运行特定测试
ctest -R AccountManagerTest

# 显示详细输出
ctest --output-on-failure
```

### 方法3: 直接运行可执行文件

```bash
cd build

# 运行单个测试
./test_account_manager

# 运行特定测试用例
./test_account_manager --gtest_filter=AccountManagerTest.BasicBalanceOperations

# 显示所有测试用例
./test_account_manager --gtest_list_tests
```

## 测试覆盖

### AccountManager 测试

测试覆盖：
- ✅ 基本余额操作（设置、更新、查询）
- ✅ 可用余额计算
- ✅ 冻结/解冻操作
- ✅ 余额充足性检查
- ✅ 保证金计算
- ✅ 账户统计
- ✅ 多用户场景
- ✅ 边界情况
- ✅ 线程安全

### PositionManager 测试

测试覆盖：
- ✅ 持仓操作（买入、卖出）
- ✅ 多空持仓管理
- ✅ 持仓限制检查
- ✅ 新持仓大小计算
- ✅ 多品种、多用户场景
- ✅ 持仓平仓
- ✅ 边界情况

### OrderBook 测试

测试覆盖：
- ✅ 订单插入
- ✅ 最佳买卖价
- ✅ 价差计算
- ✅ 订单删除
- ✅ 空检查
- ✅ 匹配检查
- ✅ 同价位多订单
- ✅ 价格层级排序
- ✅ 订单查找
- ✅ 边界情况

### OrderValidator 测试

测试覆盖：
- ✅ 有效订单验证
- ✅ 无效价格/数量验证
- ✅ 无效订单ID/用户ID验证
- ✅ 价格范围验证
- ✅ 数量范围验证
- ✅ 价格精度验证
- ✅ 数量步长验证
- ✅ 多规则组合验证

### LiquidationEngine 测试

测试覆盖：
- ✅ 风险等级计算
- ✅ 清算触发检查
- ✅ 维持保证金计算
- ✅ 低保证金清算场景
- ✅ 风险比率计算
- ✅ 无持仓场景

### FundingRateManager 测试

测试覆盖：
- ✅ 资金费率计算
- ✅ 溢价指数计算
- ✅ 结算时间管理
- ✅ 资金费率边界检查
- ✅ 多品种管理

### MatchingEngineEventSourcing 测试

测试覆盖：
- ✅ 订单处理与事件溯源
- ✅ 订单撮合
- ✅ 事件回放
- ✅ 确定性模式
- ✅ 订单取消
- ✅ 多订单场景

## 编写新测试

### 测试文件模板

```cpp
#include <gtest/gtest.h>
#include "core/your_component.h"
#include "core/types.h"

using namespace perpetual;

class YourComponentTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 初始化测试环境
        component_ = std::make_unique<YourComponent>();
    }
    
    void TearDown() override {
        // 清理测试环境
    }
    
    std::unique_ptr<YourComponent> component_;
};

// 基本功能测试
TEST_F(YourComponentTest, BasicFunctionality) {
    // Arrange: 准备测试数据
    // Act: 执行操作
    // Assert: 验证结果
    EXPECT_TRUE(component_->someMethod());
}

// 边界情况测试
TEST_F(YourComponentTest, EdgeCases) {
    // 测试边界情况
}

// 错误处理测试
TEST_F(YourComponentTest, ErrorHandling) {
    // 测试错误处理
}
```

### 测试命名规范

- 测试类名: `ComponentNameTest`
- 测试用例名: `DescriptiveTestName`
- 使用下划线分隔单词

示例：
```cpp
TEST_F(AccountManagerTest, BasicBalanceOperations)
TEST_F(AccountManagerTest, FreezeUnfreezeOperations)
TEST_F(AccountManagerTest, ThreadSafety)
```

### 测试原则

1. **AAA 模式**: Arrange（准备）、Act（执行）、Assert（断言）
2. **独立性**: 每个测试应该独立，不依赖其他测试
3. **可重复性**: 测试应该可以重复运行并得到相同结果
4. **清晰性**: 测试名称和代码应该清晰表达测试意图
5. **完整性**: 覆盖正常路径、边界情况和错误情况

## 最佳实践

### 1. 测试组织

- 每个核心组件都有对应的测试文件
- 相关测试用例分组到同一个测试类中
- 使用 `SetUp()` 和 `TearDown()` 进行测试环境管理

### 2. 断言使用

```cpp
// 相等性检查
EXPECT_EQ(expected, actual);
ASSERT_EQ(expected, actual);  // 失败时立即停止

// 布尔值检查
EXPECT_TRUE(condition);
EXPECT_FALSE(condition);

// 浮点数比较（考虑精度）
EXPECT_DOUBLE_EQ(expected, actual);
EXPECT_NEAR(value, expected, tolerance);

// 空指针检查
EXPECT_NE(ptr, nullptr);
EXPECT_EQ(ptr, nullptr);
```

### 3. 测试数据

- 使用有意义的测试数据
- 避免硬编码魔法数字
- 使用辅助函数创建测试数据

### 4. 错误消息

```cpp
EXPECT_TRUE(result) << "Detailed error message";
EXPECT_EQ(a, b) << "Expected " << a << " but got " << b;
```

### 5. 性能考虑

- 单元测试应该快速运行
- 避免在单元测试中进行性能测试
- 使用性能测试目录进行性能相关测试

### 6. 线程安全测试

```cpp
TEST_F(AccountManagerTest, ThreadSafety) {
    const int num_threads = 4;
    const int operations_per_thread = 100;
    std::vector<std::thread> threads;
    std::atomic<int> success_count{0};
    
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&]() {
            // 并发操作
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    // 验证结果
}
```

## 故障排查

### 编译错误

```bash
# 清理构建目录
rm -rf build
mkdir build
cd build
cmake ..
make
```

### 链接错误

- 检查 CMakeLists.txt 中的链接库配置
- 确保所有依赖项都已正确链接

### 运行时错误

```bash
# 使用详细输出运行测试
./test_account_manager --gtest_output=xml:results.xml

# 使用调试器
gdb ./test_account_manager
```

### 测试失败

1. 查看测试输出获取详细错误信息
2. 检查测试数据是否正确
3. 验证被测试组件的实现
4. 检查是否有环境依赖问题

## 持续集成

建议在 CI/CD 流程中：

1. 每次提交时运行所有单元测试
2. 测试失败时阻止合并
3. 生成测试覆盖率报告
4. 跟踪测试执行时间

## 相关文档

- [Google Test 文档](https://google.github.io/googletest/)
- [CMake 测试文档](https://cmake.org/cmake/help/latest/manual/ctest.1.html)
- [项目架构文档](../ARCHITECTURE.md)

## 贡献

添加新测试时：

1. 遵循现有的测试结构和命名规范
2. 确保测试通过
3. 更新本文档的测试覆盖部分
4. 提交前运行所有测试确保没有破坏现有功能

