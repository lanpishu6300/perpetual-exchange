# Perpetual Exchange - 永续合约交易所核心引擎

一个高性能的永续合约交易所撮合引擎，专注于纳秒级延迟优化。

## 核心特性

### 🚀 高性能设计
- **纳秒级延迟**: 采用高效的数据结构和算法优化
- **红黑树订单簿**: O(log n) 复杂度的订单插入、删除和查询
- **价格层级聚合**: 相同价格的订单聚合，减少内存占用和查询时间
- **缓存友好**: 数据结构对齐到缓存行，优化CPU缓存命中率

### 📊 核心功能
- **订单撮合**: 价格时间优先撮合算法
- **订单簿管理**: 支持深度查询和实时更新
- **仓位管理**: 支持多空仓位、平均开仓价计算
- **保证金计算**: 实时保证金和风险控制
- **资金费率**: 永续合约资金费率计算和结算
- **风险控制**: 强制平仓价格计算和风险检查

## 项目结构

```
perpetual_exchange/
├── include/
│   └── core/
│       ├── types.h           # 基础类型定义
│       ├── order.h           # 订单结构
│       ├── orderbook.h       # 订单簿实现
│       ├── matching_engine.h # 撮合引擎
│       ├── position.h        # 仓位管理
│       ├── account.h         # 账户管理
│       └── funding_rate.h    # 资金费率计算
├── src/
│   └── core/
│       ├── orderbook.cpp
│       ├── matching_engine.cpp
│       ├── position.cpp
│       ├── account.cpp
│       └── funding_rate.cpp
├── src/
│   └── main.cpp              # 示例程序
├── CMakeLists.txt
└── README.md
```

## 构建说明

### 系统要求
- C++17 或更高版本
- CMake 3.15 或更高版本
- 支持 C++17 的编译器 (GCC 7+, Clang 5+, MSVC 2017+)

### 构建步骤

```bash
# 创建构建目录
mkdir build && cd build

# 配置项目
cmake ..

# 编译
cmake --build . --config Release

# 运行示例
./bin/PerpetualExchange
```

## 核心设计

### 订单簿结构
- 使用红黑树维护价格优先级
- 每个价格层级聚合相同价格的所有订单
- 订单在价格层级内按时间优先排列

### 撮合算法
1. **价格优先**: 最优价格优先撮合
2. **时间优先**: 相同价格按时间戳排序
3. **支持订单类型**: 限价单、市价单、IOC、FOK

### 仓位计算
- **双向持仓**: 支持同时持有多空仓位
- **净持仓**: 自动计算净持仓大小
- **平均开仓价**: 加权平均开仓价格计算
- **未实现盈亏**: 基于标记价格实时计算

### 资金费率
- **溢价指数**: (标记价格 - 指数价格) / 指数价格
- **资金费率**: 溢价指数 × 资金费率系数
- **资金费用**: 持仓量 × 标记价格 × 资金费率

## 性能优化

### 数据结构优化
- 使用整数类型存储价格和数量（定点数），避免浮点运算
- 订单结构对齐到64字节（缓存行大小）
- 使用内存池减少动态分配

### 算法优化
- 红黑树保证O(log n)的操作复杂度
- 价格层级预聚合，减少遍历次数
- 最小化锁的使用，提高并发性能

## 使用示例

```cpp
#include "core/matching_engine.h"

// 创建撮合引擎
MatchingEngine engine(instrument_id);

// 设置回调
engine.set_trade_callback([](const Trade& trade) {
    // 处理成交事件
});

// 创建订单
auto order = std::make_unique<Order>(
    order_id, user_id, instrument_id,
    OrderSide::BUY, price, quantity
);

// 处理订单
auto trades = engine.process_order(order.get());
```

## 参考

本项目参考了以下业界领先交易所的设计：
- **Binance**: 价格时间优先撮合
- **Deribit**: 永续合约资金费率机制
- **OKX**: 订单簿深度管理
- **Bybit**: 风险控制和强制平仓

## 许可证

本项目仅供学习和研究使用。

## 贡献

欢迎提交 Issue 和 Pull Request！
